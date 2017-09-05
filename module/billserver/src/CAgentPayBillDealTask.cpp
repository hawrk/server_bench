/*
 * 
 CAgentPayBillDealTask.cpp
 *
 *  Created on: 2009-6-3
 *      Author: 
 */

#include <sys/time.h>
#include "CAgentPayBillDealTask.h"
#include "tools.h"
#include "msglogapi.h"
#include "../../Base/Comm/UserInfoClient.h"
#include "log/clog.h"
#include "../business/apayErrorNo.h"

extern TMsgLog g_stMsgLog;


INT32 CAgentPayBillDealTask::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
{
    BEGIN_LOG(__func__);

    gettimeofday(&m_stStart, NULL);
    Reset();

    if ( !m_bInited )
    {
        snprintf( m_szErrMsg, sizeof(m_szErrMsg), "Not Inited" );
        m_iRetCode = -1;
        return m_iRetCode;
    }

	try
	{
	    //获取请求
	    FillReq(mapInput);

	    //校验请求
	    CheckInput();

		//业务处理
		Deal();

		SetRetParam();
	}
	catch(CTrsExp e)
	{
		m_RetMap.clear();
		m_RetMap["err_code"] = e.retcode;
		m_RetMap["err_msg"]  = e.retmsg;
	}

	BuildResp( outbuf, outlen );

    return atoi(m_RetMap["err_code"].c_str());
}


/*
 * 解析出请求结构
 * 成功返回0 失败返回-1
 */
void CAgentPayBillDealTask::FillReq( NameValueMap& mapInput)
{
    BEGIN_LOG(__func__);

	MapFirstToLower(mapInput,m_InParams);
}

void CAgentPayBillDealTask::CheckInput()
{
	BEGIN_LOG(__func__);

	Check::CheckIsDigitalParam("settle_date",m_InParams["settle_date"],8,8);

	if(!m_InParams["settle_date"].empty())
	{
		if(!ISDIGIT(m_InParams["settle_date"].c_str())
			|| STOI(m_InParams["settle_date"]) >= STOI(GetDate()))
		{
			CERROR_LOG("settle_date[%s] param is error!", m_InParams["settle_date"].c_str());
			throw(CTrsExp(ERR_INVALID_PARAMS, "settle_date param is error!"));	
		}	
	}
	else
	{
		m_InParams["settle_date"] = GetYesterDate();
	}

	Check::CheckDigitalParam("step",m_InParams["step"],1,3);

	if(m_InParams["step"].empty())
	{
		m_InParams["step"] = "3";
	}	
}

void CAgentPayBillDealTask::BuildResp( CHAR** outbuf, INT32& outlen )
{
	BEGIN_LOG(__func__);

    CHAR szResp[ MAX_RESP_LEN ];
	JsonMap jsonRsp; 

	if(m_RetMap["err_code"].empty())
	{
		m_RetMap["err_code"] = "00";
		m_RetMap["err_msg"]  = "";
	}
	else
	{		
		Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig()->ReadErrCodeFromFile(m_RetMap);
	}

	jsonRsp.insert(JsonMap::value_type(JsonType("err_code"), JsonType(m_RetMap["err_code"])));
	jsonRsp.insert(JsonMap::value_type(JsonType("err_msg"), JsonType(m_RetMap["err_msg"])));

	
	std::string resContent = JsonUtil::objectToString(jsonRsp);

	//resContent.erase(std::remove(resContent.begin(), resContent.end(), '\\'), resContent.end());
	CDEBUG_LOG("resContent=[%s]!\n",resContent.c_str());

    gettimeofday(&m_stEnd, NULL);
    INT32 iUsedTime = CalDiff(m_stStart, m_stEnd);
	CDEBUG_LOG("process use time=[%d]!\n",iUsedTime);
	snprintf(szResp, sizeof(szResp), //remaincount=1
		"%s\r\n",
		resContent.c_str());

	outlen = strlen(szResp);
	*outbuf = (char*)malloc(outlen);
	memcpy(*outbuf, szResp, outlen);
}

void CAgentPayBillDealTask::GetAndLoadBillFile()
{
	BEGIN_LOG(__func__);

	pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();

	char szLoadFileCmd[512] = { 0 };
	snprintf(szLoadFileCmd, sizeof(szLoadFileCmd), "sh %s%s %s",
		pBillBusConfig->apayMainConf.loadBillFileShPath.c_str(), pBillBusConfig->apayMainConf.loadBillFileShName.c_str(),
		m_InParams["settle_date"].c_str());
	CDEBUG_LOG("szLoadFileCmd = [%s]",szLoadFileCmd);

	system(szLoadFileCmd);
}

//对账操作
void CAgentPayBillDealTask::CompareDeal()
{
	BEGIN_LOG(__func__);

	CompareSucce();

	CompareRefund();
}

//对成功单
void CAgentPayBillDealTask::CompareSucce()
{
	BEGIN_LOG(__func__);
	
	stringstream szSqlBuf("");
	SqlResultMapVector resMVector;
	StrSqlResultSetMap localOrderMap;
	StrSqlResultSetMap webankOrderMap;
	resMVector.clear();
	localOrderMap.clear();
	webankOrderMap.clear();

	//查询本地账单
	string szDBname = string(AGENT_PAY_ORDER_DB) + string("_") + m_InParams["settle_date"].substr(0,6);
	string szTablename = APAY_ORDER_TABLE;

	QryLocalBill(szDBname,szTablename,localOrderMap);

	//如果对账日期是月初1号,需跨库查询,特殊处理
	//需额外查询上个月的库表
	if(m_InParams["settle_date"].substr(6,2) == "01")
	{
		//查询上个月库表
		INT32 iMonTemp = STOI(m_InParams["settle_date"].substr(0,6));
		szDBname = string(AGENT_PAY_ORDER_DB) + string("_") + ITOS(iMonTemp-1);
		szTablename = APAY_ORDER_TABLE;
		
		QryLocalBill(szDBname,szTablename,localOrderMap);
	}

	//查询微众账单DB
	QryWeBankBill(AGENT_PAY_BILL_DB,APAY_WEBANK_BILL_TABLE,webankOrderMap);

	INT32 iLocalBillSize  = localOrderMap.size();
	INT32 iWeBankBillSize = webankOrderMap.size();

	//两边都没有订单，无法对账
	if(iLocalBillSize == 0 && iWeBankBillSize == 0)
	{
		CDEBUG_LOG("date=[%s] had no trade data!",m_InParams["settle_date"].c_str());
		throw(CTrsExp(ERR_APAY_NOT_BILL_DATA,"该天没有交易,无法完成对账"));
	}
	else if(iLocalBillSize > 0 && iWeBankBillSize == 0)
	{
		//本地有交易订单，而微众对账单为空，抛错
		CDEBUG_LOG("date=[%s] webank's bill data is empty!",m_InParams["settle_date"].c_str());
		throw(CTrsExp(ERR_APAY_WEBANK_BILL_EMPTY,"微众对账单为空"));
	}

	//一笔一笔订单对比
	map<string, SqlResultSet>::iterator localIter = localOrderMap.begin();
	for(;localIter!=localOrderMap.end();localIter++)
	{
		//CDEBUG_LOG("webankOrderMap's size=[%d]",webankOrderMap.size());
		string szOrderNo = localIter->first;
		SqlResultSet  orderSet = localIter->second;

		map<string, SqlResultSet>::iterator webIter = webankOrderMap.find(szOrderNo);

		if(webIter == webankOrderMap.end())
		{
			if(orderSet["order_status"] == APAY_ORDER_STATUS_FAILED)
			{
				//失败的订单，不在微众对账单中，属于正常
				continue;
			}	
			else if((orderSet["order_status"] == APAY_ORDER_STATUS_SUCCESS
					  || orderSet["order_status"] == APAY_ORDER_STATUS_DEALING)
					&& orderSet["settle_date"] == m_InParams["settle_date"])
			{
				NameValueMap inMap,resMap;
								
				inMap["order_no"] = szOrderNo;
				inMap["order_status"] = APAY_ORDER_STATUS_FAILED;
				CallAgentPayServer(inMap,resMap);
				//UpdateOrder(orderSet,APAY_ORDER_STATUS_FAILED);
			}			
		}
		else
		{
		 	m_FeeCnt += STOL(orderSet["payment_fee"]);
			m_NumCnt += 1;
		
			SqlResultSet webankSet = webIter->second;
			//CDEBUG_LOG("orderId=%s",webankSet["orderId"].c_str());
		
			if(orderSet["order_status"] ==  APAY_ORDER_STATUS_DEALING 
				|| orderSet["order_status"] == APAY_ORDER_STATUS_FAILED)
			{
				//处理中or失败的订单，在微众的对账单，需更改状态为成功
				NameValueMap inMap,resMap;

				inMap["order_no"] = szOrderNo;
				inMap["order_status"] = APAY_ORDER_STATUS_SUCCESS;
				inMap["settle_date"] = webankSet["settleDate"];
				CallAgentPayServer(inMap,resMap);
				//UpdateOrder(orderSet,APAY_ORDER_STATUS_SUCCESS);
			}
			else if(orderSet["order_status"] == APAY_ORDER_STATUS_SUCCESS)
			{
				//成功的单，进行对账
				bool wellDone = false;
				string szErrMsg = "";

				//对比商户号
				if(orderSet["mch_agentpay_acct_id"] != webankSet["merId"])
				{
					szErrMsg = "商户号不一致";
				}				
				else if(orderSet["payment_fee"] != Y2F(webankSet["amount"]))
				{
					//对比金额
					szErrMsg = "交易金额不一致";
				}
				else if(webankSet["status"] != APAY_ORDER_STATUS_SUCCESS)
				{
					//对比状态
					szErrMsg = "微众账单状态不为成功";
				}
				else
				{
					wellDone = true;
				}

				if(wellDone)
				{
					//对账成功的单，把微众的删掉
					//webankOrderMap.erase(webIter++);
					//webankOrderMap.erase(szOrderNo);
				}
				else
				{
					//异常帐，写入数据库
					//orderSet["reqDate"] = webankSet["reqDate"];
					AbnormalOrderDeal(webankSet,szErrMsg);
				}
			}


			//对账完成的单，把微众的删掉
			webankOrderMap.erase(webIter++);
		}
	}

	CDEBUG_LOG("after compare deal webankOrderMap's size=[%d]",webankOrderMap.size());	
	if(webankOrderMap.size() > 0 )
	{
		//对完账后，微众有多出来的单，保存到异常表
		map<string, SqlResultSet>::iterator webankIter = webankOrderMap.begin();
		for(;webankIter!=webankOrderMap.end();webankIter++)
		{
			SqlResultSet webankSet = webankIter->second;

			if(webankSet["reqDate"] != webankSet["settleDate"])
			{
				//如果对账日期跟请求日期不一致,说明是跨天交易的对账
				DealAcrossDayBill(webankSet);
			}
			else
			{
				string szErrMsg = "微众账单中多的订单";
				
				AbnormalOrderDeal(webankSet,szErrMsg);
			}
		}
	}	
}

//对退票单
void CAgentPayBillDealTask::CompareRefund()
{
	BEGIN_LOG(__func__);

	stringstream szSqlBuf("");
	SqlResultMapVector resMVector;

	//查询微众账单
	szSqlBuf.str("");
	szSqlBuf	<<	"SELECT merId,orderId,amount,reqDate,settleDate,CASE status WHEN 'PR00' THEN 1 WHEN 'PR02' THEN 3 WHEN 'PR03' THEN 2 END AS status,respCode,respMsg"
				<<	" FROM "
				<<	AGENT_PAY_BILL_DB	<<	"." <<	APAY_WEBANK_REFUND_BILL_TABLE
				<<	" WHERE "
				<<	"settleDate='"	<<	m_InParams["settle_date"]	<<	"';";

	clib_mysql* cMysql =  Singleton<CSpeedPosConfig>::GetInstance()->GetApayBillDB();
	m_SqlHandle.QryAndFetchResMVector(*cMysql,szSqlBuf.str().c_str(),resMVector);

	INT32 iVecSize = resMVector.size();
	CDEBUG_LOG("webank refund bill resMVector's size=[%d]",iVecSize);

	if(iVecSize <= 0)
	{
		CDEBUG_LOG("no webank refund bill found!");
		return;
	}

	vector<SqlResultSet>::iterator vIter = resMVector.begin();
	for(;vIter != resMVector.end();vIter++)
	{
		SqlResultSet webankRefSet = *vIter;

		SqlResultSet orderSet;
		NameValueMap inMap;

		inMap["order_no"] = webankRefSet["orderId"];
		QryOrder(inMap,orderSet);

		//记录退票异常的相关数据
		string szErrMsg = "";
/*		
		SqlResultSet reqMap;
		reqMap["order_no"] = webankRefSet["orderId"];
		reqMap["mch_agentpay_acct_id"] = webankRefSet["merId"];
		reqMap["payment_fee"] = Y2F(webankRefSet["amount"]);
		reqMap["order_status"] = webankRefSet["status"];
		reqMap["settle_date"]  = webankRefSet["settleDate"];
		reqMap["create_time"] = webankRefSet["reqDate"];
*/
		//对账正常标识
		bool wellDone = false;
		
		if(orderSet.size() <= 0)
		{
			//没有找到原单，插入异常表
			szErrMsg = "退票异常单,无法找到原交易单";
			AbnormalOrderDeal(webankRefSet,szErrMsg);
		}
		else
		{
			m_RefFeeCnt += STOL(orderSet["payment_fee"]);
			m_RefNumCnt += 1;
		
			//对比商户号
			if(orderSet["mch_agentpay_acct_id"] != webankRefSet["merId"])
			{
				szErrMsg = "退票对账,商户号不一致";
			}				
			else if(orderSet["payment_fee"] != Y2F(webankRefSet["amount"]))
			{
				//对比金额
				szErrMsg = "退票对账,交易金额不一致";
			}
			else
			{
				wellDone = true;
			}

			if(wellDone)
			{
				//更改状态
				if(orderSet["order_status"] != APAY_ORDER_STATUS_REFUND)
				{
					NameValueMap reqMap,resMap;
									
					reqMap["order_no"] = webankRefSet["orderId"];
					reqMap["order_status"] = APAY_ORDER_STATUS_REFUND;
					CallAgentPayServer(reqMap,resMap);				
					//UpdateOrder(inMap,APAY_ORDER_STATUS_REFUND);
				}
			}
			else
			{
				//对账异常，插入异常记录表
				AbnormalOrderDeal(webankRefSet,szErrMsg);	
			}
		}
	}
}

//查询本地账单DB
void CAgentPayBillDealTask::QryLocalBill(string szDbName,string szTableName,StrSqlResultSetMap & localOrderMap)
{
	BEGIN_LOG(__func__);

	stringstream szSqlBuf("");
	SqlResultMapVector resMVector;
	resMVector.clear();

	//查询本地账单
	string szSqlString1 = "";
	string szSqlString2 = "";

	szSqlBuf	<<	" SELECT order_no,out_order_no,order_type,biz_type,mch_id,pay_channel_id,mch_agentpay_acct_id,payment_fee,order_status,settle_date"
				<<	" FROM ";
	szSqlString1 = szSqlBuf.str();

	szSqlBuf.str("");
	szSqlBuf	<<	" WHERE "
				<<	" ((create_time >= date_format("	<<	m_InParams["settle_date"]	<<	"000000,'%Y-%m-%d %H:%i:%s')"
				<<	" AND create_time <= date_format("	<<	m_InParams["settle_date"]	<<	"235959,'%Y-%m-%d %H:%i:%s'))"
				<<	" OR settle_date='" <<	m_InParams["settle_date"]	<<	"')"
				<<	" AND order_type="	<<	ORDER_TYPE_APAY
				<<	" AND pay_channel_id="	<<	APAY_CHL_WEBANK 
				<<	" AND pyh_status="	<<	APAY_ORDER_PYH_STATUS_VALID <<	";";
	szSqlString2 = szSqlBuf.str();

	CDEBUG_LOG("szSqlString1=[%s],szSqlString2=[%s]",szSqlString1.c_str(),szSqlString2.c_str());

	//根据链接池循环查询每个DB的10张表
	CDBPool* pDbPool = Singleton<CSpeedPosConfig>::GetInstance()->GetApayDBPool();
	QryTableLoop(pDbPool,m_SqlHandle,szDbName,szTableName,szSqlString1,szSqlString2,resMVector);

	INT32 iLocalVecSize = resMVector.size();
	CDEBUG_LOG("local order resMVector's size=[%d]",iLocalVecSize);

	if(iLocalVecSize <= 0)
	{
		CDEBUG_LOG("no order found!");
		//return;
	}
	
	for(int i=0;i<iLocalVecSize;i++)
	{
		SqlResultSet tempMap = resMVector[i];
		localOrderMap[tempMap["order_no"]] = tempMap;
	}
	CDEBUG_LOG("localOrderMap's size=[%d]",localOrderMap.size());	
}

//查询微众账单DB
void CAgentPayBillDealTask::QryWeBankBill(string szDbName,string szTableName,StrSqlResultSetMap & webankOrderMap)
{
	BEGIN_LOG(__func__);

	stringstream szSqlBuf("");
	SqlResultMapVector resMVector;
	resMVector.clear();

	//查询微众账单
	szSqlBuf.str("");
	szSqlBuf	<<	"SELECT merId,orderId,amount,reqDate,settleDate,CASE status WHEN 'PR00' THEN 1 WHEN 'PR02' THEN 3 WHEN 'PR03' THEN 2 ELSE -1 END AS status,respCode,respMsg"
				<<	" FROM "
				<<	AGENT_PAY_BILL_DB	<<	"." <<	APAY_WEBANK_BILL_TABLE
				<<	" WHERE "
				<<	"settleDate='"	<<	m_InParams["settle_date"]	<<	"';";

	clib_mysql* cMysql =  Singleton<CSpeedPosConfig>::GetInstance()->GetApayBillDB();
	m_SqlHandle.QryAndFetchResMVector(*cMysql,szSqlBuf.str().c_str(),resMVector);

	INT32 iWeBankVecSize = resMVector.size();
	CDEBUG_LOG("webank bill resMVector's size=[%d]",iWeBankVecSize);

	if(iWeBankVecSize <= 0)
	{
		CDEBUG_LOG("no webank bill found!");
		//return;
	}

	for(int i=0;i<iWeBankVecSize;i++)
	{
		SqlResultSet tempMap = resMVector[i];
		webankOrderMap[tempMap["orderId"]] = tempMap;
	}
	CDEBUG_LOG("webankOrderMap's size=[%d]",webankOrderMap.size()); 	
}

//更新订单状态
void CAgentPayBillDealTask::UpdateOrder(SqlResultSet& inMap,string szStatus)
{
	BEGIN_LOG(__func__);

	NameValueMap tempMap;
	ParseOrderNo(inMap["order_no"],tempMap);
	
	stringstream szSqlBuf("");
	string szSql = "";

	if(szStatus == APAY_ORDER_STATUS_SUCCESS)
	{
		szSql += "pay_time=now(),";
	}

	szSqlBuf	<<	" UPDATE "
				<<	AGENT_PAY_ORDER_DB	<<	"_"	<<	tempMap["db_name"]	<<	"." <<	APAY_ORDER_TABLE	<<	"_"	<<	tempMap["table_name"]
				<<	" SET "
				<<	"order_status="	<<	szStatus	<<	","
				<<	szSql
				<<	"modify_time=now(),modify_memo='compare bill update status'"
				<<	" WHERE "
				<<	"order_no='"	<<	inMap["order_no"] <<	"';";

	CDEBUG_LOG("sqlString=[%s]!",szSqlBuf.str().c_str());

	CDBPool* pDbPool = Singleton<CSpeedPosConfig>::GetInstance()->GetApayDBPool();
	std::map<int, clib_mysql*> DbConMap = pDbPool->GetMasterDBPool();
	clib_mysql* cMysql = DbConMap[STOI(tempMap["dbpool_no"])];

	try
	{
		//开启事务
		m_SqlHandle.Begin(*cMysql);

		m_SqlHandle.Execute(*cMysql,szSqlBuf.str().c_str());

		//提交
		m_SqlHandle.Commit(*cMysql);

		if(m_SqlHandle.getAffectedRows() != 1)
		{
			CERROR_LOG("update db error,roolback!sql=[%s]!\n",szSqlBuf.str().c_str());
			throw(CTrsExp(UPDATE_DB_ERR,"db update error!"));
		}
	}
	catch(CTrsExp e)
	{
		//回滚
		m_SqlHandle.Rollback(*cMysql);
		throw e;
	}

	try
	{
		NameValueMap reqMap;
		reqMap["order_no"] = inMap["order_no"];

		//同步到RabbitMQ
		CallRabbitMQNew(reqMap);
	}
	catch(...)
	{
		// do nothing 
	}
}

//异常账处理
void CAgentPayBillDealTask::AbnormalOrderDeal(SqlResultSet& inMap,string szErrMsg)
{
	BEGIN_LOG(__func__);

	m_BillFlag = false;

	stringstream szSqlBuf("");

	inMap["order_no"] = inMap["orderId"];
	inMap["mch_agentpay_acct_id"] = inMap["merId"];
	inMap["payment_fee"] = Y2F(inMap["amount"]);
	inMap["order_status"] = inMap["status"];
	inMap["settle_date"]  = inMap["settleDate"];
	inMap["create_date"]  = inMap["reqDate"];
	inMap["bill_date"]    = GetDate();

	szSqlBuf.str("");
	szSqlBuf	<<	"INSERT INTO "
				<<	AGENT_PAY_BILL_DB	"." <<	APAY_WEBANK_BILL_ABNORMAL_TABLE
				<<	" SET "
				<<	"order_no='"	<<	inMap["order_no"]	<<	"',"
				<<	"mch_agentpay_acct_id='"	<<	inMap["mch_agentpay_acct_id"]	<<	"',"
				<<	"payment_fee="	<<	inMap["payment_fee"]	<<	","
				<<	"order_status="	<<	inMap["order_status"]	<<	","
				<<	"settle_date='"	<<	inMap["settle_date"]	<<	"',"
				<<	"create_date='"	<<	inMap["create_date"]	<<	"',"
				<<	"bill_date='"	<<	inMap["bill_date"]	<<	"',"
				<<	"err_msg='"	<<	szErrMsg	<<	"';";

	clib_mysql* cMysql =  Singleton<CSpeedPosConfig>::GetInstance()->GetApayBillDB();

	try
	{
		//开启事务
		m_SqlHandle.Begin(*cMysql);

		m_SqlHandle.Execute(*cMysql,szSqlBuf.str().c_str());

		//提交
		m_SqlHandle.Commit(*cMysql);

		if(m_SqlHandle.getAffectedRows() != 1)
		{
			CERROR_LOG("update db error,roolback!sql=[%s]!\n",szSqlBuf.str().c_str());
			throw(CTrsExp(UPDATE_DB_ERR,"db update error!"));
		}
	}
	catch(CTrsExp e)
	{
		//回滚
		m_SqlHandle.Rollback(*cMysql);
		throw e;
	}	
}


//查询订单
void CAgentPayBillDealTask::QryOrder(NameValueMap& inMap,SqlResultSet &outMap)
{
	BEGIN_LOG(__func__);

	NameValueMap tempMap;
	ParseOrderNo(inMap["order_no"],tempMap);

	stringstream szSqlBuf("");

	string szDbName = string(AGENT_PAY_ORDER_DB) + string("_") + tempMap["db_name"];
	string szTabName = string(APAY_ORDER_TABLE) + "_" + tempMap["table_name"];

	string szSqlString = "";

	szSqlBuf	<<	" SELECT order_no,out_order_no,mch_id,biz_type,pay_channel_id,mch_agentpay_acct_id,order_status,payment_fee,notify_url"
				<<	" FROM "
				<<	szDbName	<<	"." <<	szTabName
				<<	" WHERE order_no='" <<	inMap["order_no"]	<<	"';";

	CDBPool* pDbPool = Singleton<CSpeedPosConfig>::GetInstance()->GetApayDBPool();
	std::map<int, clib_mysql*> DbConMap = pDbPool->GetMasterDBPool();

	clib_mysql* m_DBConn = DbConMap[STOI(tempMap["dbpool_no"])];
	INT32 iRet = m_SqlHandle.QryAndFetchResMap(*m_DBConn,szSqlBuf.str().c_str(),outMap);

	if(iRet != 1)
	{/*
		CERROR_LOG("order_no[%s] is not exist!\n",inMap["order_no"].c_str());
		throw(CTrsExp(ERR_APAY_ORDER_NOT_EXISTS,"order_no is not exist!"));*/
	}	
}

//生成商户对账文件
void CAgentPayBillDealTask::CreateMchBillFile()
{
	BEGIN_LOG(__func__);

	string szBillPath = pBillBusConfig->apayMainConf.MchBillFilePath.c_str();

	if(szBillPath.empty())
	{
		CERROR_LOG("mch bill file path is empty!");
		throw(CTrsExp(SYSTEM_ERR,"mch bill file path is empty!"));		
	}

	//判断存放商户账单的目录是否存在
	if(access(szBillPath.c_str(),0) != 0)
	{
		CERROR_LOG("mch bill file path is not exist!");
		throw(CTrsExp(SYSTEM_ERR,"mch bill file path is not exist!"));
	}

	//判断对账日对应的文件夹是否存在
	string  szSetDatePath = szBillPath + "/" + m_InParams["settle_date"];
	if(access(szSetDatePath.c_str(),0) != 0)
	{
		//不存在则创建
		if( mkdir(szSetDatePath.c_str(),S_IRWXU|S_IRWXG|S_IROTH) != 0)
		{
			CERROR_LOG("cann't create path[%s]!",szSetDatePath.c_str());
			throw(CTrsExp(SYSTEM_ERR,"cann't create bill path"));
		}
	}
	
	//获取数据库中成功的单
	stringstream szSqlBuf("");
	SqlResultMapVector orderMVector;
	orderMVector.clear();


	//查询本地账单
	string szDBname = string(AGENT_PAY_ORDER_DB) + string("_") + m_InParams["settle_date"].substr(0,6);
	string szTablename = APAY_ORDER_TABLE;

	string szSqlString1 = "";
	string szSqlString2 = "";

	szSqlBuf	<<	" SELECT order_no,out_order_no,mch_id,payment_fee,summar_code,order_status,date_format(create_time,'%Y%m%d') AS trade_date"
				<<	" FROM ";
	szSqlString1 = szSqlBuf.str();

	szSqlBuf.str("");
	szSqlBuf	<<	" WHERE create_time >= date_format("	<<	m_InParams["settle_date"]	<<	"000000,'%Y-%m-%d %H:%i:%s')"
				<<	" AND create_time <= date_format("	<<	m_InParams["settle_date"]	<<	"235959,'%Y-%m-%d %H:%i:%s')"
				<<	" AND order_status IN ("	<<	APAY_ORDER_STATUS_SUCCESS	<<	","	<<	APAY_ORDER_STATUS_REFUND	<<	")"	
				<<	" AND order_type="	<<	ORDER_TYPE_APAY
				<<	" AND pay_channel_id="	<<	APAY_CHL_WEBANK
				<<	" AND pyh_status="	<<	APAY_ORDER_PYH_STATUS_VALID	<<	";";
	szSqlString2 = szSqlBuf.str();

	CDEBUG_LOG("szSqlString1=[%s],szSqlString2=[%s]",szSqlString1.c_str(),szSqlString2.c_str());

	//根据链接池循环查询每个DB的10张表
	CDBPool* pDbPool = Singleton<CSpeedPosConfig>::GetInstance()->GetApayDBPool();
	QryTableLoop(pDbPool,m_SqlHandle,szDBname,szTablename,szSqlString1,szSqlString2,orderMVector);

	INT32 iVecSize = orderMVector.size();
	CDEBUG_LOG("order orderMVector's size=[%d]",iVecSize);
	
	ofstream iFile;
	string szMchFilePath = szSetDatePath + "/agentpay_bill_" + m_InParams["settle_date"]+ ".txt";
	iFile.open(szMchFilePath.c_str(),ios::in|ios::trunc);
	if(!iFile)
	{
		iFile.close();
		CERROR_LOG("cann't create file[%s]!",szMchFilePath.c_str());
		throw(CTrsExp(SYSTEM_ERR,"cann't create bill file!"));
	}
	
	vector<SqlResultSet>::iterator vIter = orderMVector.begin();
	for(;vIter != orderMVector.end();vIter++)
	{
		SqlResultSet orderSet = *vIter;
		/*
		map<string,string>::iterator mIter = orderSet.begin();
		for(;mIter != orderSet.end();mIter++)
		{
			if(mIter == orderSet.begin())
			{
				iFile<<mIter->second;
			}
			else
			{
				iFile<<","<<mIter->second;
			}
		}*/

		iFile<<orderSet["order_no"]<<","<<orderSet["out_order_no"]<<","<<orderSet["mch_id"]<<","<<orderSet["payment_fee"]\
		<<","<<orderSet["summar_code"]<<","<<orderSet["order_status"]<<","<<orderSet["trade_date"]<<","<<m_InParams["settle_date"]<<"\n";
	}
	
	iFile<<"###END###\n";
	iFile.close();	
}

//处理跨天交易的对账单
void CAgentPayBillDealTask::DealAcrossDayBill(SqlResultSet & sqlSet)
{
	BEGIN_LOG(__func__);

	NameValueMap tempMap;
	ParseOrderNo(sqlSet["orderId"],tempMap);

	//查询本地账单
	string szDBname = string(AGENT_PAY_ORDER_DB) + string("_") + tempMap["db_name"];
	string szTablename = string(APAY_ORDER_TABLE) + string("_") + tempMap["table_name"];
	
	stringstream szSqlBuf("");
	SqlResultSet orderSet;

	szSqlBuf	<<	" SELECT order_no,out_order_no,order_type,biz_type,mch_id,pay_channel_id,mch_agentpay_acct_id,payment_fee,order_status"
				<<	" FROM "
				<<	szDBname	<<	"."	<<	szTablename
				<<	" WHERE order_no='"	<<	sqlSet["orderId"]	<<	"'"
				<<	" AND order_type="	<<	ORDER_TYPE_APAY
				<<	" AND pay_channel_id="	<<	APAY_CHL_WEBANK	
				<<	" AND pyh_status="	<<	APAY_ORDER_PYH_STATUS_VALID	<<	";";

	CDBPool* pDbPool = Singleton<CSpeedPosConfig>::GetInstance()->GetApayDBPool();
	std::map<int, clib_mysql*> DbConMap = pDbPool->GetMasterDBPool();
	clib_mysql* cMysql = DbConMap[STOI(tempMap["dbpool_no"])];

	m_SqlHandle.QryAndFetchResMap(*cMysql,szSqlBuf.str().c_str(),orderSet);

	bool wellDone = false;
	string szErrMsg = "";
	
	if(orderSet.size() <= 0)
	{
		szErrMsg = "微众账单中多的订单";
		CERROR_LOG("order_no[%s] is not exist!\n",sqlSet["orderId"].c_str());
		//throw(CTrsExp(ERR_APAY_ORDER_NOT_EXISTS,"order_no is not exist!"));
	}
	else
	{
		//进行对账
		
		//对比商户号
		if(orderSet["mch_agentpay_acct_id"] != sqlSet["merId"])
		{
			szErrMsg = "商户号不一致";
		}				
		else if(orderSet["payment_fee"] != Y2F(sqlSet["amount"]))
		{
			//对比金额
			szErrMsg = "交易金额不一致";	
		}
		else if(sqlSet["status"] != APAY_ORDER_STATUS_SUCCESS)
		{
			//对比状态
			szErrMsg = "微众账单状态不为成功";
		}
		else if(orderSet["order_status"] != APAY_ORDER_STATUS_SUCCESS)
		{
			//订单不为成功，则同步
			NameValueMap inMap,resMap;
							
			inMap["order_no"] = sqlSet["orderId"];
			inMap["order_status"] = APAY_ORDER_STATUS_SUCCESS;
			inMap["settle_date"] = sqlSet["settleDate"];
			CallAgentPayServer(inMap,resMap);
			//UpdateOrder(orderSet,APAY_ORDER_STATUS_FAILED);
		
			wellDone = true;
		}
		else
		{
			wellDone = true;
		}
	}

	if(wellDone)
	{
		//do  nothing
	}
	else
	{
		//异常帐，写入数据库
		AbnormalOrderDeal(sqlSet,szErrMsg);
	}	
}

//查询对账日志表
void CAgentPayBillDealTask::QryBillLog()
{
	BEGIN_LOG(__func__);

	SqlResultSet billLogMap;

	stringstream szSqlBuf("");

	szSqlBuf	<<	" SELECT settle_date,bill_date,pay_channel_id,total_fee,total_number,bill_status"
				<<	" FROM "
				<<	AGENT_PAY_BILL_DB	<<	"." <<	APAY_BILL_LOG_TABLE
				<<	" WHERE "
				<<	"settle_date='" <<	m_InParams["settle_date"]	<<	"'"
				<<	" AND "
				<<	"pay_channel_id='"	<<	APAY_CHL_WEBANK	<<	"';";

	clib_mysql* m_DBConn = Singleton<CSpeedPosConfig>::GetInstance()->GetApayBillDB();
	INT32 iRet = m_SqlHandle.QryAndFetchResMap(*m_DBConn,szSqlBuf.str().c_str(),billLogMap);

	if(iRet == 1)
	{
		if(billLogMap["bill_status"] == APAY_BILL_STATUS_SUCCESS)
		{
			CERROR_LOG("bill_status[%s] is invalid!", billLogMap["bill_status"].c_str());
			throw(CTrsExp(ERR_APAY_BILL_STATUS, "bill_status[%s] is invalid!"));
		}
		else
		{
			//存在则更新
			szSqlBuf.str("");
			szSqlBuf	<<	" UPDATE "
						<<	AGENT_PAY_BILL_DB	<<	"." <<	APAY_BILL_LOG_TABLE
						<<	" SET "
						<<	"bill_status="	<<	APAY_BILL_STATUS_DEALING	<<	","
						<<	"modify_time=now()"
						<<	" WHERE "
						<<	"settle_date='" <<	m_InParams["settle_date"]	<<	"'"
						<<	" AND "
						<<	"pay_channel_id='"	<<	APAY_CHL_WEBANK	<<	"'";				
		}										
	}
	else
	{
		//不存在，则新增
		szSqlBuf.str("");
		szSqlBuf	<<	" INSERT INTO "
					<<	AGENT_PAY_BILL_DB	<<	"." <<	APAY_BILL_LOG_TABLE
					<<	" SET "
					<<	"settle_date='"	<<	m_InParams["settle_date"]	<<	"',"
					<<	"bill_date='"	<<	GetDate()	<<	"',"
					<<	"pay_channel_id='"	<<	APAY_CHL_WEBANK	<<	"',"
					<<	"pay_channel_name='"	<<	"微众银行"	<<	"',"
					<<	"bill_status="	<<	APAY_BILL_STATUS_DEALING	<<	","
					<<	"create_time=now();";
	}

	try
	{
		//开启事务
		m_SqlHandle.Begin(*m_DBConn);

		m_SqlHandle.Execute(*m_DBConn,szSqlBuf.str().c_str());

		//提交
		m_SqlHandle.Commit(*m_DBConn);

		if(m_SqlHandle.getAffectedRows() != 1)
		{
			CERROR_LOG("insert db error,roolback!sql=[%s]!\n",szSqlBuf.str().c_str());
			throw(CTrsExp(INSERT_DB_ERR,"db insert error!"));
		}
	}
	catch(CTrsExp e)
	{
		//回滚
		m_SqlHandle.Rollback(*m_DBConn);
		throw e;
	}	
}

//更新对账状态
void CAgentPayBillDealTask::UpdateBillLog(bool bFlag,string msg)
{
	BEGIN_LOG(__func__);

	SqlResultSet billLogMap;

	stringstream szSqlBuf("");
	string szBillStatus = "";
	string szMsg = "";
		
	if(bFlag)
	{
		szBillStatus = APAY_BILL_STATUS_SUCCESS;
		szMsg = "对账成功";
	}
	else
	{
		szBillStatus = APAY_BILL_STATUS_FAILED;
		szMsg = msg.empty() ? "对账异常" : msg;
	}

	szSqlBuf	<<	" UPDATE "
				<<	AGENT_PAY_BILL_DB	<<	"." <<	APAY_BILL_LOG_TABLE
				<<	" SET "
				<<	"total_fee="	<<	m_FeeCnt	<<	","
				<<	"total_number="	<<	m_NumCnt	<<	","
				<<	"total_ref_fee="	<<	m_RefFeeCnt	<<	","
				<<	"total_ref_number="	<<	m_RefNumCnt	<<	","
				<<	"bill_status="	<<	szBillStatus	<<	","
				<<	"err_msg='"	<<	szMsg	<<	"',"
				<<	"modify_time=now()"
				<<	" WHERE "
				<<	"settle_date='" <<	m_InParams["settle_date"]	<<	"'"
				<<	" AND "
				<<	"pay_channel_id='"	<<	APAY_CHL_WEBANK	<<	"';";

	clib_mysql* m_DBConn = Singleton<CSpeedPosConfig>::GetInstance()->GetApayBillDB();

	try
	{
		//开启事务
		m_SqlHandle.Begin(*m_DBConn);

		m_SqlHandle.Execute(*m_DBConn,szSqlBuf.str().c_str());

		//提交
		m_SqlHandle.Commit(*m_DBConn);

		if(m_SqlHandle.getAffectedRows() != 1)
		{
			CERROR_LOG("update db error,roolback!sql=[%s]!\n",szSqlBuf.str().c_str());
			throw(CTrsExp(UPDATE_DB_ERR,"db update error!"));
		}
	}
	catch(CTrsExp e)
	{
		//回滚
		m_SqlHandle.Rollback(*m_DBConn);
		throw e;
	}	
}


void CAgentPayBillDealTask::Deal()
{
	BEGIN_LOG(__func__);

	QryBillLog();

	try
	{
		switch (STOI(m_InParams["step"].c_str()))
		{
			case 1:
					GetAndLoadBillFile();
					break;
			case 2:
					GetAndLoadBillFile();
					CompareDeal();
					break;
			case 3:
					GetAndLoadBillFile();
					CompareDeal();
					//CreateMchBillFile();
					break;
			default:
					CERROR_LOG("step[%s] param is error!", m_InParams["step"].c_str());
					throw(CTrsExp(ERR_INVALID_PARAMS, "step param is error!"));	
		}
	}
	catch(CTrsExp e)
	{
		m_BillFlag = false;

		NameValueMap reqMap;
		reqMap["err_code"] = e.retcode;
		Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig()->ReadErrCodeFromFile(reqMap);
		
		UpdateBillLog(m_BillFlag,reqMap["err_msg"]);
		throw e;
	}

	UpdateBillLog(m_BillFlag,"");
}

void CAgentPayBillDealTask::SetRetParam()
{
	BEGIN_LOG(__func__);

}


void CAgentPayBillDealTask::LogProcess()
{
   
}

