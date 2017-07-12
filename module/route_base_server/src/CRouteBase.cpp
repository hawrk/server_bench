#include "CRouteBase.h"

string GetMchKey(const string& mch_id) throw(CTrsExp)
{
//	int iRet = 0;
//	ostringstream sqlss;
//	SqlResultSet mchInfo;
//
//	sqlss.str("");
//	sqlss << "select mch_id,bm_id,organ_id,mch_key,shop_name from "
//		<< SHOP_DB << "." << SHOPS_TABLE << " where "
//		<< "mch_id = '" << mch_id << "'"
//		<< ";";
//	clib_mysql * pShopDB = Singleton<CSpeedPosConfig>::GetInstance()->GetShopDB();
//	CMySQL mysql;
//	iRet = mysql.QryAndFetchResMap(*pShopDB, sqlss.str().c_str(), mchInfo);
//	if ( 0 == iRet )
//	{
//		CERROR_LOG("mch_id[%s] not exists!", mch_id.c_str());
//        throw(CTrsExp(ERR_MCH_NOT_EXISTS, "mch_id not exists!"));
//	}
//
//	return mchInfo["mch_key"];
	return "";
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

void CallIdServer(NameValueMap& reqMap,NameValueMap& resMap)throw(CTrsExp)
{
	CSocket* idSocket = Singleton<CSpeedPosConfig>::GetInstance()->GetIdServerSocket();

	NameValueMap resultMap;
    
    //char buf[10240] = {0};
	//idSocket->SendAndRecv(reqMap,buf,10240);
   
    //string szResString = string(buf);
    //Kv2Map(szResString,resultMap);

    idSocket->SendAndRecvLine(reqMap,resultMap,"\r\n");

	MapFirstToLower(resultMap,resMap);
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



