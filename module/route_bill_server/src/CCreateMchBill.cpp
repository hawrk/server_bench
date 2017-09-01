/*
 * CCreateMchBill.cpp
 *
 *  Created on: 2017年7月21日
 *      Author: hawrkchen
 */

#include "CCreateMchBill.h"

CCreateMchBill::CCreateMchBill()
{
	//ctor
}

CCreateMchBill::~CCreateMchBill()
{
	//dector
}

INT32 CCreateMchBill::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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

	    CheckMchBillTask();

	    CalcEffectiveTimeBill();

	    CreateMchBill();

	    CreateChanBill();

	    AccountingCheckin();


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
void CCreateMchBill::FillReq( NameValueMap& mapInput)
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

void CCreateMchBill::CheckInput()
{

	if(m_InParams["ver"].empty() || m_InParams["ver"] != VER)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("bill_date", m_InParams["bill_date"], 1, 10, true);  //对账单日期
	//Check::CheckStrParam("pay_channel", m_InParams["pay_channel"], 1, 10, true);  //对账通道


}

INT32 CCreateMchBill::CalcEffectiveTimeBill()
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

void CCreateMchBill::CheckMchBillTask()
{
	int iRet = 0;
	SqlResultSet outMap;
	sqlss.str("");
	sqlss <<"SELECT Fbill_date,Fpay_channel_id "
		  <<" FROM "<<ROUTE_BILL_DB<<"."<<MCH_CHECK_BILL
		  <<" WHERE Fbill_date = '"<<m_InParams["bill_date"]<<"';";

	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),outMap);
	if(iRet == 1)  //
	{
		CERROR_LOG("current date :[%s] has finish !!...",m_InParams["bill_date"].c_str());
    	throw(CTrsExp(ERR_MCH_BILL_EXIST,"当日商户对账单已生成！"));
	}
}

void CCreateMchBill::CreateMchBill()
{
	int iRet;
	SqlResultMapVector resMVector;
	//SqlResultMapVector mchMVector;
	//商户订单清分冻结  （目前只有成功）
	sqlss.str("");
	sqlss <<"SELECT Forder_no,Fmch_id,Fpay_channel_id,Fpay_channel,Ftotal_amount,Fmch_rate_val FROM "
		  <<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
		  <<" where Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time
		  <<"' AND Forder_status ='"<<ORDER_SUCCESS<<"';";

	iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),resMVector);
	if(iRet == 1)
	{
		for(size_t i = 0; i < resMVector.size(); i++)
		{
			MchBillSum mchbillsum;
			mchbillsum.Reset();
			mchbillsum.order_no         = resMVector[i]["Forder_no"];
			mchbillsum.mch_id 			= resMVector[i]["Fmch_id"];
			mchbillsum.channel_id 		= resMVector[i]["Fpay_channel_id"];
			mchbillsum.channel_name 	= resMVector[i]["Fpay_channel"];
			mchbillsum.total_amount 	= resMVector[i]["Ftotal_amount"] == "" ? 0:atol(resMVector[i]["Ftotal_amount"].c_str());
			mchbillsum.total_fee 		= resMVector[i]["Fmch_rate_val"] == "" ? 0:atol(resMVector[i]["Fmch_rate_val"].c_str());    //手续费
			mchbillsum.net_amount       = atol(resMVector[i]["Ftotal_amount"].c_str());   //无退款，交易净额 = 支付金额
			mchbillsum.pending_amount   = atol(resMVector[i]["Ftotal_amount"].c_str()) - atol(resMVector[i]["Fmch_rate_val"].c_str());  //结算金额

			InserIntoMchBill(mchbillsum,FUND_TYPE_MCH);
		}
	}

}

void CCreateMchBill::CreateChanBill()
{
	CDEBUG_LOG("GetChannelData begin ................................");

	int iRet;
	SqlResultMapVector resMVector;
	MchBillSum mchsum;

	//
	sqlss.str("");
	sqlss <<"SELECT Forder_no,Fmch_id,Fpay_channel_id,Fagent_id,Ftotal_amount,Fchannel_profit FROM "
		  <<ROUTE_BILL_DB<<"."<<T_ROUTE_CHANNEL
		  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time
		  <<"' AND Forder_status = 'SUCCESS';";
	iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),resMVector);
	if(iRet == 1)
	{
		for(size_t i = 0; i < resMVector.size(); i++)
		{
			MchBillSum mchbillsum;
			mchbillsum.Reset();

			mchbillsum.order_no         = resMVector[i]["Forder_no"];
			mchbillsum.mch_id 			= resMVector[i]["Fagent_id"];
			mchbillsum.channel_id 		= resMVector[i]["Fpay_channel_id"];
			//mchbillsum.channel_name 	= resMVector[i]["Fpay_channel"];
			mchbillsum.total_amount 	= resMVector[i]["Ftotal_amount"] == "" ? 0:atol(resMVector[i]["Ftotal_amount"].c_str());
			mchbillsum.pending_amount   = resMVector[i]["Fchannel_profit"] == "" ? 0:atol(resMVector[i]["Fchannel_profit"].c_str());  //结算金额

			InserIntoMchBill(mchbillsum,FUND_TYPE_CH);

		}
	}
}

void CCreateMchBill::InserIntoMchBill(MchBillSum& mch_bill,const char* fund_type)
{
	int iRet = 0;
//	mch_bill.total_count = mch_bill.total_count.empty() ? "0":mch_bill.total_count;
//	mch_bill.total_amount = mch_bill.total_amount.empty() ? "0" :mch_bill.total_amount;
//	mch_bill.refund_count = mch_bill.refund_count.empty() ? "0" :mch_bill.refund_count;
//	mch_bill.refund_amount = mch_bill.refund_amount.empty() ? "0" :mch_bill.refund_amount;
//	mch_bill.net_amount = mch_bill.net_amount.empty() ? "0" :mch_bill.net_amount;
//	mch_bill.total_fee = mch_bill.total_fee.empty() ? "0" :mch_bill.total_fee;

	sqlss.str("");
	sqlss <<"INSERT INTO "<<ROUTE_BILL_DB<<"."<<MCH_CHECK_BILL
		  <<" (Forder_no,Ffund_type,Ffund_id,Fbill_date,Fpay_channel_id,Fpay_channel,Ftrade_amount,"
		  <<"Frefund_amount,Fnet_amount,Ftotal_fee,Fpending_amount,Faccount_type,Faccount_status) values('"<<mch_bill.order_no<<"','"
		  <<fund_type<<"','"<<mch_bill.mch_id<<"','"<<m_InParams["bill_date"]<<"','"<<mch_bill.channel_id<<"','"<<mch_bill.channel_name
		  <<"',"<<mch_bill.total_amount<<","<<mch_bill.refund_amount
		  <<","<<mch_bill.net_amount<<","<<mch_bill.total_fee<<","<<mch_bill.pending_amount<<",'2201','0');";

    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
    if(iRet != 1)
    {
    	CERROR_LOG("insert t_mch_check_bill fail!!!");
    	throw(CTrsExp(INSERT_DB_ERR,"insert t_mch_check_bill fail!!!"));
    }

}

void CCreateMchBill::AccountingCheckin()
{
	int iRet;
	char szRecvBuff[1024] = {0};
	StringMap paramMap;
	StringMap recvMap;
	SqlResultMapVector billInfoVector;
	CSocket* accountSocket = Singleton<CSpeedPosConfig>::GetInstance()->GetAccountServerSocket();

	string strAccListID;

    sqlss.str("");
    sqlss <<"select Forder_no,Ffund_type,Fpay_channel_id,Ffund_id,Fpending_amount from "
    	  <<ROUTE_BILL_DB<<"."<<MCH_CHECK_BILL
		  <<" where  Fbill_date = '"<<m_InParams["bill_date"]<<"';";

    iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),billInfoVector);
    if(iRet == 1)  //有记录
    {
		for(unsigned int i = 0; i < billInfoVector.size(); i++)
		{
			if(atoi(billInfoVector[i]["Fpending_amount"].c_str()) <= 0)
			{
				continue;
			}
			strAccListID.clear();
			strAccListID = GetAccountSeqNo();
			paramMap.clear();
			memset(szRecvBuff,0x00,sizeof(szRecvBuff));
			paramMap.insert(StringMap::value_type("cmd","1008"));
			paramMap.insert(StringMap::value_type("ver","1.0"));

			JsonMap  bizCJsonMap;
			bizCJsonMap.insert(JsonMap::value_type(JsonType("order_no"), billInfoVector[i]["Forder_no"]));           //订单号
			bizCJsonMap.insert(JsonMap::value_type(JsonType("accounting_type"), "2201"));
			bizCJsonMap.insert(JsonMap::value_type(JsonType("total_fee"),billInfoVector[i]["Fpending_amount"]));    //转入金额
			bizCJsonMap.insert(JsonMap::value_type(JsonType("tran_fee"), billInfoVector[i]["Fpending_amount"]));   //交易金额
			bizCJsonMap.insert(JsonMap::value_type(JsonType("account_list_id"),strAccListID ));    //记账流水
			bizCJsonMap.insert(JsonMap::value_type(JsonType("from_user_id"), billInfoVector[i]["Ffund_id"])); //转出账号
			bizCJsonMap.insert(JsonMap::value_type(JsonType("from_user_type"), billInfoVector[i]["Ffund_type"]));  //转出方用户类型
			//bizCJsonMap.insert(JsonMap::value_type(JsonType("from_type"), "CNY"));
			bizCJsonMap.insert(JsonMap::value_type(JsonType("to_user_id"), billInfoVector[i]["Ffund_id"]));  //转入账号
			bizCJsonMap.insert(JsonMap::value_type(JsonType("to_user_type"), billInfoVector[i]["Ffund_type"]));  //转入方用户类型
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
				UpdateCheckStatus(billInfoVector[i]["Forder_no"],billInfoVector[i]["Ffund_id"],strAccListID,ACCOUNT_SUCCESS,ret_msg);
			}
			else
			{
				UpdateCheckStatus(billInfoVector[i]["Forder_no"],billInfoVector[i]["Ffund_id"],strAccListID,ACCOUNT_FAIL,ret_msg);

			}
			CDEBUG_LOG(">>>>>>>>>>>>>end>>>>>>>>>>>>>>>>");

		}
    }

}

void CCreateMchBill::UpdateCheckStatus(const string& order_no,const string& fund_id,const string& account_no,const string& acc_status,const string& acc_desc)
{
	//根据账单日期和用户类型确认一笔交易
	int iRet = 0;
	sqlss.str("");
	//更新记账流水和记账状态
	sqlss <<" UPDATE "<<ROUTE_BILL_DB<<"."<<MCH_CHECK_BILL
		  <<" SET Faccount_serial_no = '"<<account_no<<"', Faccount_status = '"<<acc_status
		  <<"',Faccount_desc = '"<<acc_desc
		  <<"',Fmodify_time = now() where Forder_no ='"<<order_no
		  <<"' and Ffund_id = '"<<fund_id<<"';";

    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
    if(iRet != 1)
    {
    	CERROR_LOG("update t_mch_check_bill fail!!!");
    	throw(CTrsExp(UPDATE_DB_ERR,"update t_mch_check_bill fail!!!"));
    }

}

std::string CCreateMchBill::GetAccountSeqNo()
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

void CCreateMchBill::SetRetParam()
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

void CCreateMchBill::BuildResp( CHAR** outbuf, INT32& outlen )
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

void CCreateMchBill::LogProcess()
{
}
