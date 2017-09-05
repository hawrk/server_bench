#include "CAgentPayBase.h"

 

string GetMchKey(const string& mch_id) throw(CTrsExp)
{
	int iRet = 0;
	ostringstream sqlss;
	SqlResultSet mchInfo;

	sqlss.str("");
	sqlss << "select mch_id,bm_id,organ_id,mch_key,shop_name from " 
		<< SHOP_DB << "." << SHOPS_TABLE << " where "
		<< "mch_id = '" << mch_id << "'"
		<< ";";
	clib_mysql * pShopDB = Singleton<CSpeedPosConfig>::GetInstance()->GetShopDB();
	CMySQL mysql;
	iRet = mysql.QryAndFetchResMap(*pShopDB, sqlss.str().c_str(), mchInfo);
	if ( 0 == iRet )
	{
		CERROR_LOG("mch_id[%s] not exists!", mch_id.c_str());
        throw(CTrsExp(ERR_MCH_NOT_EXISTS, "mch_id not exists!"));
	}

	return mchInfo["mch_key"];
}

string GetMchSign(const string& mch_id,const string& szSrc) throw(CTrsExp)
{
	string szKey = GetMchKey(mch_id);
	string strSrcStr = ""; // 签名源串

	strSrcStr += szSrc + "&key=" + szKey;
	CDEBUG_LOG("strSrcStr=[%s]", strSrcStr.c_str());
	string newSign = getMd5(strSrcStr);	

	return newSign;
}


void CheckMD5Sign(NameValueMap& nvMap) throw(CTrsExp)
{
	string signature = nvMap["signature"];
	string strSrcStr; // 签名源串
	
	Map2Kv(nvMap, strSrcStr, "|sign_flag|signature|cmd|");
	strSrcStr += string("&key=") + GetMchKey(nvMap["mch_id"]);
	CDEBUG_LOG("strSrcStr=[%s]", strSrcStr.c_str());
	string newSign = getMd5(strSrcStr);

	if ( signature != newSign )
	{
		CERROR_LOG("in sign[%s] != generate sign[%s].", signature.c_str(), newSign.c_str());
		throw(CTrsExp(ERR_CHECK_MCH_SIGN_FAIL, "check mch sign fail!"));
	}
}


void CallAgentPayServer(NameValueMap& reqMap,NameValueMap& resMap)throw(CTrsExp)
{
	CSocket* apaySocket = Singleton<CSpeedPosConfig>::GetInstance()->GetApayServerSocket();

	NameValueMap resultMap;
	reqMap["ver"] = "1.0";
	reqMap["cmd"] = APAY_SERVER_ORDER_MODIFY;

	char buf[10240] = {0};
	//apaySocket->SendAndRecv(reqMap,buf,10240);
	apaySocket->SendAndRecvLineEx(reqMap,buf,10240,"\r\n");

	JsonMap resJsonMap;
	resJsonMap = JsonType(JsonUtil::stringToObject(string(buf))).toMap();

	//apaySocket->SendAndRecvLine(reqMap,resultMap,"\r\n");
	CDEBUG_LOG("call agentpay_server success,err_code=[%s],err_msg=[%s]!", resJsonMap[JsonType("err_code")].toString().c_str(), resJsonMap[JsonType("err_msg")].toString().c_str());

	MapFirstToLower(resultMap,resMap);

	if(resJsonMap[JsonType("err_code")].toString() != "00")
	{
		CERROR_LOG("call agentpay_server error,err_code=[%s],err_msg=[%s]!", resJsonMap[JsonType("err_code")].toString().c_str(), resJsonMap[JsonType("err_msg")].toString().c_str());
		throw(CTrsExp(ERR_CALL_APAY_SERVER, "call agentpay_server error!"));
	}
}


void QryTableLoop(CDBPool* pDbPool,CMySQL &sqlHandle,string & szDbName,string &szTable,string &szSqlBuf1,string &szSqlBuf2,SqlResultMapVector &resultMVecter)
{
	//切换DB机器10张表，循环查询
	//CDBPool* pDbPool = Singleton<CSpeedPosConfig>::GetInstance()->GetDBPool();
	std::map<int, clib_mysql*> DbConMap = pDbPool->GetMasterDBPool();
	for (std::map<int, clib_mysql*>::iterator iter = DbConMap.begin(); iter != DbConMap.end(); ++iter)
	{
		clib_mysql* pDbCon = iter->second;
		for (int iDbIndex = 0; iDbIndex < 10; ++iDbIndex)
		{
			SqlResultMapVector tempMVecter;
			CDEBUG_LOG("pDbCon[%p] host[%s]! iDbIndex[%d]. \n", pDbCon, pDbCon->ms_host, iDbIndex);

			stringstream szSqlString("");
			szSqlString	<<	szSqlBuf1	<<	szDbName	<<	"."	<<	szTable	<<	"_"	<<	iDbIndex	<<	szSqlBuf2;
			
			//CMySQL cSqlHandle;
			sqlHandle.QryAndFetchResMVector(*pDbCon,szSqlString.str().c_str(),tempMVecter);

			vector<SqlResultSet>::iterator vIter = tempMVecter.begin();

			for(;vIter != tempMVecter.end();vIter++)
			{
				resultMVecter.push_back(*vIter);
			}			
		}
	
	}
}

//返回值:0-成功；其他-失败
INT32 CallRabbitMQNew(NameValueMap& reqMap)
{
	if(reqMap["order_no"].empty())
	{
		CDEBUG_LOG("reqMap's order_no is empty!");
		return -1;
	}

	NameValueMap tempMap;
	ParseOrderNo(reqMap["order_no"],tempMap);

	stringstream szSqlBuf("");
	SqlResultSet orderMap;

	szSqlBuf	<<	" SELECT "
				<<	"order_no,out_order_no,order_type,biz_type,mch_id,mch_name,pay_channel_id,pay_channel_name,mch_agentpay_acct_id,"
				<<	"cur_type,payment_fee,payment_profit,payee_acct_no,payee_acct_name,card_type,payee_bank_no,payee_bank_name,"
				<<	"summar_code,payee_phone_num,payee_acct_type,id_type,id_num,order_status,err_msg,create_time,pay_time,settle_date,modify_time"
				<<	" FROM "
				<<	AGENT_PAY_ORDER_DB	<<	"_"	<<	tempMap["db_name"]	<<	"." <<	APAY_ORDER_TABLE	<<	"_"	<<	tempMap["table_name"]
				<<	" WHERE "
				<<	"order_no='"	<<	reqMap["order_no"]	<<	"';";

	CMySQL mySql;

	CDBPool* pDbPool = Singleton<CSpeedPosConfig>::GetInstance()->GetApayDBPool();
	std::map<int, clib_mysql*> DbConMap = pDbPool->GetMasterDBPool();

	//m_DBConn = Singleton<CSpeedPosConfig>::GetInstance()->GetAentpayConfDB();
	clib_mysql* DBconn = DbConMap[STOI(tempMap["dbpool_no"])];
	
	mySql.QryAndFetchResMap(*DBconn,szSqlBuf.str().c_str(),orderMap);
	
	JsonMap dataJMap;
	NameValueMapIter iter;
	for(iter=orderMap.begin();iter!=orderMap.end();iter++)
	{
		if(iter->first == "order_type"
			||iter->first == "biz_type"
			||iter->first == "payment_fee"
			||iter->first == "payment_profit"
			||iter->first == "card_type"
			||iter->first == "payee_acct_type"
			||iter->first == "id_type"
			||iter->first == "order_status"
			||iter->first == "pyh_status")
		{
			dataJMap.insert(JsonMap::value_type(JsonType(iter->first), JsonType(STODOUBLE(iter->second))));
		}
		else
		{
			dataJMap.insert(JsonMap::value_type(JsonType(iter->first), JsonType(iter->second)));
		}
	}

	if(reqMap["insert_flag"] == "1")
	{
		dataJMap.insert(JsonMap::value_type(JsonType("option"), JsonType("agent_insert")));
	}
	else
	{
		dataJMap.insert(JsonMap::value_type(JsonType("option"), JsonType("agent_update")));
	}

	CDEBUG_LOG("begin sync to RabbitMQ!");

	std::string content = JsonUtil::objectToString(dataJMap);

	//CDEBUG_LOG("sync rabbitMQ data[%s]!\n",content.c_str());
	
	CRabbitMQClient* client = Singleton<CSpeedPosConfig>::GetInstance()->GetRabbitMQ();
	INT32 iRet = client->Push(content, Singleton<CSpeedPosConfig>::GetInstance()->GetRabbitMQKey());

	if(iRet == 0)
	{
		CDEBUG_LOG("sync to RabbitMQ success ! iRet=[%d]",iRet);

		//更新同步标识
		szSqlBuf.str("");

		szSqlBuf	<<	" UPDATE "
					<<	AGENT_PAY_ORDER_DB	<<	"_"	<<	tempMap["db_name"]	<<	"." <<	APAY_ORDER_TABLE	<<	"_"	<<	tempMap["table_name"]		
					<<	" SET "
					<<	"sync_flag="	<<	APAY_ORDER_SYNC_DONE	<<	","
					<<	"modify_time=now(),modify_memo='sync to rabbitMQ success.'"
					<<	" WHERE "
					<<	"order_no='"	<<	reqMap["order_no"]	<<	"';";

		try
		{
			//开启事务
			mySql.Begin(*DBconn);

			mySql.Execute(*DBconn,szSqlBuf.str().c_str());

			//提交
			mySql.Commit(*DBconn);

			if(mySql.getAffectedRows() != 1)
			{
				CERROR_LOG("insert db error,roolback!sql=[%s]!\n",szSqlBuf.str().c_str());
				throw(CTrsExp(INSERT_DB_ERR,"db insert error!"));
			}
		}
		catch(CTrsExp e)
		{
			//回滚
			mySql.Rollback(*DBconn);
			throw e;
		}
	}
	else
	{
		CDEBUG_LOG("sync to RabbitMQ fialed ! iRet=[%d]",iRet);	
	}
	
	return 0;
}


INT32 CallRabbitMQ(JsonMap& reqMap)
{
	std::string content = JsonUtil::objectToString(reqMap);

	CDEBUG_LOG("sync rabbitMQ data[%s]!\n",content.c_str());
	
	CRabbitMQClient* client = Singleton<CSpeedPosConfig>::GetInstance()->GetRabbitMQ();
	return client->Push(content, Singleton<CSpeedPosConfig>::GetInstance()->GetRabbitMQKey());
}

void ParseOrderNo(string & szOrderNo,NameValueMap & resMap)
{
	//如果取到空值，抛错
	if(szOrderNo.empty())
	{
		CERROR_LOG("order_no in smpty!\n");
		throw(CTrsExp(SYSTEM_ERR,"Get order_no error!"));
	}

	resMap["db_name"]    = szOrderNo.substr(3,6);
	resMap["table_name"] = szOrderNo.substr(szOrderNo.length()-3,1);
	resMap["dbpool_no"]  = szOrderNo.substr(szOrderNo.length()-2,2);
}



