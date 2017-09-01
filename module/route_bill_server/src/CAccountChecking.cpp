/*
 * CAccountChecking.cpp
 *
 *  Created on: 2017年8月18日
 *      Author: hawrkchen
 *      Desc :只作清分入账用
 */

#include "CAccountChecking.h"

CAccountChecking::CAccountChecking()
{
	//ctor
}

CAccountChecking::~CAccountChecking()
{
	//dector
}

INT32 CAccountChecking::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
{
	CDEBUG_LOG("Begin ...");

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

	    CalcEffectiveTimeBill();

	    CheckAccountStatus();

	    //入账申请
		CheckinAccount();


	}
	catch(CTrsExp& e)
	{
		m_RetMap.clear();
		m_RetMap["err_code"] = e.retcode;
		m_RetMap["err_msg"]  = e.retmsg;
	}

    BuildResp( outbuf, outlen );

    CDEBUG_LOG("------------ process end----------");
    return atoi(m_RetMap["err_code"].c_str());
}

/*
 * 解析出请求结构
 * 成功返回0 失败返回-1
 */
void CAccountChecking::FillReq( NameValueMap& mapInput)
{
	NameValueMapIter iter;
	for(iter=mapInput.begin();iter!=mapInput.end();iter++)
	{
		string szName = iter->first;
		transform(szName.begin(), szName.end(), szName.begin(), ::tolower);
		//m_InParams[szName] = getSafeInput(iter->second);
		m_InParams[szName] = iter->second;
	}

	//CDEBUG_LOG("biz_content =[%s]",m_InParams["biz_content"].c_str());
//	cJSON* biz_content = cJSON_Parse(m_InParams["biz_content"].c_str());
//
//	JsonType jt = JsonUtil::stringToObject(m_InParams["biz_content"]);
//	JsonMap jm = jt.toMap();

	ParseJson2Map(m_InParams["biz_content"],m_InParams);


}

void CAccountChecking::CheckInput()
{

	if(m_InParams["ver"].empty() || m_InParams["ver"] != VER)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("bill_date", m_InParams["bill_date"], 1, 10, true);  //对账单日期
	Check::CheckStrParam("pay_channel_id", m_InParams["pay_channel_id"], 1, 20, true);  //通道号
	Check::CheckDigitalParam("type", m_InParams["type"], 1, 3, true);  //入账类型 ：1 商户 2：渠道
	Check::CheckStrParam("fund_id", m_InParams["fund_id"], 1, 20, true);  //类型为商户时传商户号，类型为渠道时传渠道号
	Check::CheckStrParam("account_type", m_InParams["account_type"], 1, 20, true);  //记账类型，目前清分入账传2202
	Check::CheckStrParam("account_amount", m_InParams["account_amount"], 1, 20, true);  //记账金额
	Check::CheckStrParam("order_no", m_InParams["order_no"], 1,32);  //订单流水

	if(m_InParams["type"] == "1")
	{
		m_InParams["fund_tpe"] = "mch";
	}
	else if(m_InParams["type"] == "2")
	{
		m_InParams["fund_tpe"] = "factor";
	}
	else
	{
		CERROR_LOG(" invalid param [type],value [%s]!",m_InParams["type"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS," type invalid!"));
	}


}

INT32 CAccountChecking::CalcEffectiveTimeBill()
{
	BEGIN_LOG(__func__);

	char szBuf[50] = { 0 };
	snprintf(szBuf, sizeof(szBuf), "%s %s", toDate(m_InParams["bill_date"]).c_str(), "00:00:00");
	m_start_time = szBuf;
	memset(szBuf,0x00,sizeof(szBuf));
	snprintf(szBuf, sizeof(szBuf), "%s %s", toDate(m_InParams["bill_date"]).c_str(), "23:59:59");
	m_end_time =szBuf;

	CDEBUG_LOG("CalcEffectiveTimeBill ! "
		"m_start_time:[%s] m_end_time:[%s].\n",
		m_start_time.c_str(),
		m_end_time.c_str());

	return 0;
}

void CAccountChecking::CheckAccountStatus()
{
	int iRet;
	SqlResultSet  OutMap;

	if(m_InParams["account_type"] == "2202")  //清分入账 2202
	{
		OutMap.clear();
		sqlss.str("");
		sqlss <<"select Faccount_status from "<<ROUTE_BILL_DB<<"."<<BILL_DISTRIBUTION
			  <<" where Fbill_date = '"<<m_InParams["bill_date"]<<"' and Fpay_channel = '"<<m_InParams["pay_channel_id"]
			  <<"' and Fmch_id = '"<<m_InParams["fund_id"]<<"';";

		iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
		if(iRet == 0)  //无数据
		{
			CERROR_LOG("CAccountChecking :t_route_bill_distribution no data!");
			throw(CTrsExp(ERR_DATA_NOT_FOUND,"无此清分数据!"));
		}
		if(OutMap["Faccount_status2"] == ACCOUNT_SUCCESS)
		{
			CERROR_LOG("CAccountChecking :account success not allow checking again!");
			throw(CTrsExp(ERR_DATA_NOT_FOUND,"该笔记录已入账，不允许重新入账!"));
		}
	}
	else
	{
		CERROR_LOG("CAccountChecking :unknow account_type!");
		throw(CTrsExp(ERR_INVALID_PARAMS,"未知的账户类型!"));
	}

}

std::string CAccountChecking::GetAccountSeqNo()
{
	//CSocket* idSocket = Singleton<CSpeedPosConfig>::GetInstance()->GetIdServerSocket();

	string strAccountSeq;
	StringMap paramMap;
	StringMap recvMap;

	paramMap.insert(StringMap::value_type("cmd","8"));

	CallIdServer(paramMap,recvMap);

	if(recvMap["retcode"] != "0")
	{
		CERROR_LOG("call id server fail!");
		throw(CTrsExp(ERR_CALL_ID_SERVER_FAIL, "call id server fail!"));
	}
	strAccountSeq = recvMap["order_no"];

	CDEBUG_LOG("Create Account Seqno :[%s]",strAccountSeq.c_str());

	return strAccountSeq;
}


void CAccountChecking::CheckinAccount()
{
	//int iRet;
	char szRecvBuff[10240] = {0};
	CSocket* accountSocket = Singleton<CSpeedPosConfig>::GetInstance()->GetAccountServerSocket();
	StringMap paramMap;
	string strAccListID = GetAccountSeqNo();

	paramMap.insert(StringMap::value_type("cmd","1008"));
	paramMap.insert(StringMap::value_type("ver","1.0"));

	JsonMap  bizCJsonMap;

	bizCJsonMap.insert(JsonMap::value_type(JsonType("order_no"), strAccListID));                     //订单号
	bizCJsonMap.insert(JsonMap::value_type(JsonType("accounting_type"), m_InParams["account_type"])); //记账类型 ：2202
	bizCJsonMap.insert(JsonMap::value_type(JsonType("total_fee"),m_InParams["account_amount"]));    //转入金额
	bizCJsonMap.insert(JsonMap::value_type(JsonType("tran_fee"), m_InParams["account_amount"]));   //交易金额
	bizCJsonMap.insert(JsonMap::value_type(JsonType("account_list_id"), strAccListID));    //记账流水
	bizCJsonMap.insert(JsonMap::value_type(JsonType("from_user_id"), m_InParams["fund_id"])); //转出账号
	bizCJsonMap.insert(JsonMap::value_type(JsonType("from_user_type"), m_InParams["fund_tpe"]));  //转出方用户类型
	//bizCJsonMap.insert(JsonMap::value_type(JsonType("from_type"), "CNY"));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("to_user_id"), m_InParams["fund_id"]));  //转入账号
	bizCJsonMap.insert(JsonMap::value_type(JsonType("to_user_type"), m_InParams["fund_tpe"]));  //转入方用户类型
	//bizCJsonMap.insert(JsonMap::value_type(JsonType("to_type"), "CNY"));

	std::string bizContent = JsonUtil::objectToString(bizCJsonMap);
	paramMap.insert(std::make_pair("biz_content", bizContent));


	accountSocket->SendAndRecvLineEx(paramMap,szRecvBuff,sizeof(szRecvBuff),"\r\n");
	CDEBUG_LOG("recv Account Msg [%s]",szRecvBuff);

	if(NULL == szRecvBuff||strlen(szRecvBuff) == 0)  //
	{
		CDEBUG_LOG("Account Check in fail !!,err_msg[%s]",szRecvBuff);
		throw(CTrsExp(ERR_CHECKIN_ACCOUNT,"请求入账操作失败!!"));
	}

	cJSON* root = cJSON_Parse(szRecvBuff);

	string ret_code = cJSON_GetObjectItem(root, "ret_code")->valuestring;
	string ret_msg = cJSON_GetObjectItem(root, "ret_msg")->valuestring;

	if(ret_code == "0")
	{
		UpdateDistributionStatus(strAccListID,ACCOUNT_SUCCESS,ret_msg);
	}
	else
	{
		UpdateDistributionStatus(strAccListID,ACCOUNT_FAIL,ret_msg);

		CERROR_LOG("ret_code:[%s] ret_msg:[%s]\n", ret_code.c_str(), ret_msg.c_str());
		throw(CTrsExp(ERR_CHECKIN_ACCOUNT,ret_msg));
	}

}

void CAccountChecking::UpdateDistributionStatus(const string& account_no,const string& acc_status,const string& acc_desc)
{
	//根据账单日期和用户类型确认一笔交易
	int iRet = 0;
	sqlss.str("");

	//更新记账流水和记账状态
	sqlss <<" UPDATE "<<ROUTE_BILL_DB<<"."<<BILL_DISTRIBUTION
		  <<" SET Faccount_serial_no = '"<<account_no<<"', Faccount_status = '"<<acc_status
		  <<"',Faccount_desc = '"<<acc_desc
		  <<"',Fmodify_time = now() where Fbill_date = '"<<m_InParams["bill_date"]<<"' and Fpay_channel = '"<<m_InParams["pay_channel_id"]
		  <<"' and Fmch_id = '"<<m_InParams["fund_id"]<<"';";


    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
    if(iRet != 1)
    {
    	CERROR_LOG("update t_route_bill_distribution fail!!!");
    	throw(CTrsExp(UPDATE_DB_ERR,"update t_route_bill_distribution fail!!!"));
    }

}

void CAccountChecking::SetRetParam()
{
//	CDEBUG_LOG("Begin ...");
//
//	for(NameValueMapIter iter = m_RetMap.begin(); iter != m_RetMap.end(); iter++)
//	{
//		m_ContentJsonMap.insert(JsonMap::value_type(JsonType(iter->first), JsonType(iter->second)));
//	}
//
//	CDEBUG_LOG("End.");
}

void CAccountChecking::BuildResp( CHAR** outbuf, INT32& outlen )
{
	CDEBUG_LOG("Begin ...");

    CHAR szResp[ MAX_RESP_LEN ];
	JsonMap jsonRsp;

	if(m_RetMap["err_code"].empty())
	{
		m_RetMap["err_code"] = "0";
		m_RetMap["err_msg"]  = "success";
	}
//	else
//    {
//        Singleton<CSpeedPosConfig>::GetInstance()->GetBusiConf()->ReadErrCodeFromFile(m_RetMap);
//    }

	jsonRsp.insert(JsonMap::value_type(JsonType("ret_code"), JsonType(m_RetMap["err_code"])));
	jsonRsp.insert(JsonMap::value_type(JsonType("ret_msg"), JsonType(m_RetMap["err_msg"])));
	jsonRsp.insert(JsonMap::value_type(JsonType("biz_content"), JsonType(m_ContentJsonMap)));

	std::string resContent = JsonUtil::objectToString(jsonRsp);

	snprintf(szResp, sizeof(szResp), //remaincount=1
		"%s\r\n",
		resContent.c_str());

	outlen = strlen(szResp);
	*outbuf = (char*)malloc(outlen);
	memcpy(*outbuf, szResp, outlen);

	CDEBUG_LOG("Rsp :[%s]",szResp);
	CDEBUG_LOG("-----------time userd:[%d ms]---------",SpeedTime());
}

void CAccountChecking::LogProcess()
{
}
