/*
 * CBillCheckTask.cpp
 *
 *  Created on: 2017年7月19日
 *      Author: hawrkchen
 */

#include "CBillCheckTask.h"

CBillCheckTask::CBillCheckTask()
{
	m_abnor_num = 0;
	m_abnor_amount = 0;
}

INT32 CBillCheckTask::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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

	    CheckBillTask();

	    LoadBillFiletoDB();


	    GetBillData();

	    CompareSuccess();

	    CompareRefund();
//	    try
//	    {
//		    m_mysql.Begin(*m_pBillDB);

		InsertIntoSummary();

		//准备清算
//		GetLiquidationData();
//
//		//写结算表
//		ProcLiquidation();

		   // m_mysql.Commit(*m_pBillDB);
//	    }
//	    catch(CTrsExp& e)
//	    {
//	    	m_mysql.Rollback(*m_pBillDB);
//			m_RetMap.clear();
//			m_RetMap["err_code"] = e.retcode;
//			m_RetMap["err_msg"]  = e.retmsg;
//	    }


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
void CBillCheckTask::FillReq( NameValueMap& mapInput)
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

void CBillCheckTask::CheckInput()
{

	if(m_InParams["ver"].empty() || m_InParams["ver"] != VER)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("bill_date", m_InParams["bill_date"], 1, 10, true);  //对账日期

	Check::CheckStrParam("pay_channel", m_InParams["pay_channel"], 1, 20, true);  //对账银行

	if(m_InParams["pay_channel"] == "GZPF" 
		|| m_InParams["pay_channel"] == "SZCIB"
		|| m_InParams["pay_channel"] == "ECITIC")
	{
		m_InParams["channel"] = "SWIFT";
	}
	else if(m_InParams["pay_channel"] == "FJHXBANK")
	{
		m_InParams["channel"] = "SPEEDPOS";
	}
	else
	{
		m_InParams["channel"] = m_InParams["pay_channel"];
	}

}

INT32 CBillCheckTask::CalcEffectiveTimeBill()
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

void CBillCheckTask::CheckBillTask()
{
	//检查对账汇总表当日是否有数据
	int iRet;
	SqlResultSet  OutMap;

	sqlss.str("");
	sqlss <<"select Fbill_date,Fpay_channel,Fbill_result from "<<ROUTE_BILL_DB<<"."<<BILL_SUMMARY
		  <<" where Fbill_date = '"<<m_InParams["bill_date"]<<"' and Fpay_channel = '"<<m_InParams["pay_channel"]
		  <<"';";
	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
	if(iRet == 1)
	{
		if(OutMap["Fbill_result"] == "1")   //对账相符，不允许再生成
		{
			CERROR_LOG("CCreateRouteBill check bill success :[%s],[%s]!",OutMap["Fbill_date"].c_str(),OutMap["Fpay_channel"].c_str());
			throw(CTrsExp(ERR_CHECK_BILL_EXIST,"当日已平账，不允许再重复对账 !"));
		}
		else   //删除汇总表数据
		{
			sqlss.str("");
			sqlss <<" DELETE FROM "
				  << ROUTE_BILL_DB<<"."<<BILL_SUMMARY
				  <<" WHERE Fbill_date = '"<<m_InParams["bill_date"]<<"' and Fpay_channel = '"<<m_InParams["pay_channel"]<<"';";

		    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
		    if(iRet != 1)
		    {
		    	CERROR_LOG("CheckBillRecord:delete t_route_bill_summary fail!!!");
		    	throw(CTrsExp(UPDATE_DB_ERR,"CheckBillRecord:delete t_route_bill_summary fail!!!"));
		    }
		}
	}
	//检查当日系统对账单是否生成
	sqlss.str("");
	sqlss <<"select Forder_no from "<<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
			  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' AND Fpay_channel_id='"
			  <<m_InParams["pay_channel"]<<"' limit 1;";

	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
	if(iRet == 0) //当日无数据，再次申请生成系统对账单
	{
		SendCreatBillRequest();
	}

}

void CBillCheckTask::SendCreatBillRequest()
{
	char szRecvBuff[1024] = {0};
	StringMap paramMap;
	StringMap recvMap;
	CSocket* billSocket = Singleton<CSpeedPosConfig>::GetInstance()->GetBillServerSocket();
	//ver=1.0&cmd=9010&bill_date=20170725
	paramMap.insert(StringMap::value_type("cmd","9010"));
	paramMap.insert(StringMap::value_type("ver","1.0"));
	paramMap.insert(StringMap::value_type("bill_date",m_InParams["bill_date"]));
	paramMap.insert(StringMap::value_type("pay_channel",m_InParams["pay_channel"]));

	billSocket->SendAndRecvLineEx(paramMap,szRecvBuff,sizeof(szRecvBuff),"\r\n");
	CDEBUG_LOG("recv Msg [%s]",szRecvBuff);

	if(NULL == szRecvBuff||strlen(szRecvBuff) == 0)  //
	{
		CDEBUG_LOG("send create bill request fail !!,err_msg[%s]",recvMap["retmsg"].c_str());
		throw(CTrsExp(SYSTEM_ERR,"send create bill request fail!!"));
	}

	cJSON* root = cJSON_Parse(szRecvBuff);
	if (0 != strcmp(cJSON_GetObjectItem(root, "ret_code")->valuestring, "0"))
	{
		CERROR_LOG("ret_code:[%s] ret_msg:[%s]\n", cJSON_GetObjectItem(root, "ret_code")->valuestring, cJSON_GetObjectItem(root, "ret_msg")->valuestring);
		//请求已 发送，生成对账单失败与否，都继续执行
		//throw(CTrsExp(SYSTEM_ERR,"send create bill request fail!!"));
	}
}

void CBillCheckTask::LoadBillFiletoDB()
{
	CRouteBillBusiConf* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBusiConf();

	//获取代理商ID
	CallPayGate2GetFactorID();

	TracateBankDB(m_InParams["channel"]);
	
	string load_tables;
	if(m_InParams["channel"] == "SWIFT")
	{
		load_tables = SWIFT_CHECK_BILL;
	}
	else if(m_InParams["channel"] == "SPEEDPOS")
	{
		load_tables = SPEEDPOS_CHENK_BILL;
	}
	else
	{
		load_tables = SZSPDB_CHECK_BILL;
	}


	for(auto factor_iter : factor_vec)
	{
		string strbill_file_name = "";
		char szLoadOrderFlowCmd[512] = { 0 };
		
		if(m_InParams["channel"] == "SPEEDPOS")
		{
			strbill_file_name = pBillBusConfig->BusiConfig.m_spdb_bill_path + factor_iter \
				+ "_" + m_InParams["bill_date"] + ".csv";
		}
		else
		{
			strbill_file_name = pBillBusConfig->BusiConfig.m_spdb_bill_path + m_InParams["pay_channel"] + "_" +
					factor_iter + "_" + m_InParams["bill_date"];

			//转换文件名称
			string strnew_file_name = strbill_file_name + ".csv";
			snprintf(szLoadOrderFlowCmd,sizeof(szLoadOrderFlowCmd),"cp %s %s",strbill_file_name.c_str(),strnew_file_name.c_str());
			CDEBUG_LOG("cp command szLoadOrderFlowCmd = [%s]",szLoadOrderFlowCmd);
			system(szLoadOrderFlowCmd);

			strbill_file_name = strnew_file_name;
		}

		CDEBUG_LOG("bill file name = [%s]",strbill_file_name.c_str());

		if(access(strbill_file_name.c_str(),F_OK) != 0)
		{
			CERROR_LOG("bill file not exist !",strbill_file_name.c_str());
			throw(CTrsExp(ERR_BILL_FILE_NOT_EXIST,"通道方对账文件不存在 !"));
		}

		if(tars::TC_File::getFileSize(strbill_file_name) == 0)
		{
			//无数据，则跳过这个文件
			CDEBUG_LOG("bill file name  [%s] no data ,jump over!!",strbill_file_name.c_str());
			continue;
		}

		string strshell_name = pBillBusConfig->BusiConfig.m_filetodb_shell;
		CDEBUG_LOG("shell name = [%s]",strshell_name.c_str());

		memset(szLoadOrderFlowCmd,0x00,sizeof(szLoadOrderFlowCmd));
		snprintf(szLoadOrderFlowCmd, sizeof(szLoadOrderFlowCmd), "sh %s %s %d %s %s %s %s %s %s",
				strshell_name.c_str(), m_pBillDB->ms_host, m_pBillDB->mi_port,
				m_pBillDB->ms_user, m_pBillDB->ms_pass, ROUTE_BILL_DB,strbill_file_name.c_str(),load_tables.c_str(),m_InParams["channel"].c_str());
		CDEBUG_LOG("sh command szLoadOrderFlowCmd = [%s]",szLoadOrderFlowCmd);
		system(szLoadOrderFlowCmd);
	}

}

void CBillCheckTask::CallPayGate2GetFactorID()
{
	char szRecvBuff[1024*10] = {0};
	factor_vec.clear();
	StringMap paramMap;
	StringMap recvMap;
	CSocket* paygateSocket = Singleton<CSpeedPosConfig>::GetInstance()->GetPaygateServerSocket();
	//调用网关服务 ，获取代理商ID
	paramMap.insert(StringMap::value_type("ver", "1"));
	paramMap.insert(StringMap::value_type("cmd","5021"));
	paramMap.insert(StringMap::value_type("version","1.0"));

	JsonMap  bizCJsonMap;
	bizCJsonMap.insert(JsonMap::value_type(JsonType("dml_type"), "ALL"));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("pay_channel_id"),m_InParams["pay_channel"]));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("group_by"),"Fchannel_factor_id"));
	std::string bizContent = JsonUtil::objectToString(bizCJsonMap);
	paramMap.insert(std::make_pair("biz_content", bizContent));

	paygateSocket->SendAndRecvLineEx(paramMap,szRecvBuff,sizeof(szRecvBuff),"\r\n");
	//CDEBUG_LOG("recv Msg [%s]",szRecvBuff);

	if(NULL == szRecvBuff||strlen(szRecvBuff) == 0)  //
	{
		CDEBUG_LOG("get factor_id fail !!,err_msg[%s]",recvMap["retmsg"].c_str());
		throw(CTrsExp(ERR_DOWNLOAD_BILL,"get factor_id fail!!"));
	}

	cJSON* root = cJSON_Parse(szRecvBuff);
	if (0 != strcmp(cJSON_GetObjectItem(root, "ret_code")->valuestring, "0"))
	{
		CERROR_LOG("ret_code:[%s] ret_msg:[%s]\n", cJSON_GetObjectItem(root, "ret_code")->valuestring, cJSON_GetObjectItem(root, "ret_msg")->valuestring);
		throw (CTrsExp(ERR_CALL_PAYGATE_SERV, cJSON_GetObjectItem(root, "ret_msg")->valuestring));
	}

	//嵌套Json
	cJSON * biz_content = cJSON_GetObjectItem(root, "biz_content");
	cJSON * factor_ids = cJSON_GetObjectItem(biz_content, "lists");
	int iTotal = cJSON_GetArraySize(factor_ids);
	CDEBUG_LOG("factor_id list [%d]",iTotal);
	for (int i = 0; i < iTotal; ++i)
	{
		cJSON* channel = cJSON_GetArrayItem(factor_ids, i);
		JsonType obj = JsonUtil::json2obj(channel);
		JsonMap jMap = obj.toMap();

		StringMap factorMap;
		factorMap["factor_id"] = jMap["channel_factor_id"].toString();
		factor_vec.push_back(factorMap["factor_id"]);
	}

	for(auto iter:factor_vec)
	{
		CDEBUG_LOG("factor_id = %s",iter.c_str());
	}
}

void CBillCheckTask::TracateBankDB(const string& channel)
{
	int iRet = 0;

	if(channel == "SWIFT")  //威富通
	{
		sqlss.str("");
		sqlss <<"DELETE FROM "<<ROUTE_BILL_DB<<"."<<SWIFT_CHECK_BILL
			  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"';";
	    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
	    if(iRet != 1)
	    {
	    	CERROR_LOG("delete  t_route_swiftpass_bill fail!!!");
	    	throw(CTrsExp(UPDATE_DB_ERR,"delete t_route_swiftpass_bill fail!!!"));
	    }
	}
	else if(channel == "SPEEDPOS")  //SPEEDPOS
	{
		sqlss.str("");
		sqlss <<"DELETE FROM "<<ROUTE_BILL_DB<<"."<<SPEEDPOS_CHENK_BILL
			  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"';";
	    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
	    if(iRet != 1)
	    {
	    	CERROR_LOG("delete  t_route_swiftpass_bill fail!!!");
	    	throw(CTrsExp(UPDATE_DB_ERR,"delete t_route_swiftpass_bill fail!!!"));
	    }
	}	
	else  //深圳浦发
	{
		sqlss.str("");
		sqlss <<"DELETE FROM "<<ROUTE_BILL_DB<<"."<<SZSPDB_CHECK_BILL
			  <<" WHERE Fbill_date = '"<<m_InParams["bill_date"]<<"';";
	    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
	    if(iRet != 1)
	    {
	    	CERROR_LOG("delete  t_route_szspdb_bill fail!!!");
	    	throw(CTrsExp(UPDATE_DB_ERR,"delete t_route_szspdb_bill fail!!!"));
	    }
	}

}

void CBillCheckTask::GetBillData()
{
	int iRet;
	SqlResultMapVector resMVector;

	//查本地对账记录
	sqlss.str("");
	sqlss <<"SELECT Forder_no,Fout_order_no,Fagent_id,Fmch_id,Fpay_channel_id,Ftransaction_id,Ftotal_amount,Frefund_amount,"
		  <<"Ffactor_rate_val,Fmch_rate_val,Fproduct_rate_val,Fplatform_rate_val,Forder_status"
		  <<" FROM "<<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
		  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' AND Fpay_channel_id='"
		  <<m_InParams["pay_channel"]<<"';";

	iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),resMVector);
	if(iRet == 1)  //有记录
	{
		for(size_t i = 0; i < resMVector.size(); i++)
		{
			if(resMVector[i]["Forder_status"] == ORDER_SUCCESS)
			{
				OrderPayBillSummary orderpaybill;
				orderpaybill.Reset();
				orderpaybill.order_no = resMVector[i]["Forder_no"];
				orderpaybill.out_order_no = resMVector[i]["Fout_order_no"];
				orderpaybill.agent_id = resMVector[i]["Fagent_id"];
				orderpaybill.mch_id = resMVector[i]["Fmch_id"];
				orderpaybill.pay_channel = resMVector[i]["Fpay_channel_id"];
				orderpaybill.transaction_id = resMVector[i]["Ftransaction_id"];
				orderpaybill.total_amount = resMVector[i]["Ftotal_amount"];
				orderpaybill.factor_rate_val = resMVector[i]["Ffactor_rate_val"];
				orderpaybill.mch_rate_val = resMVector[i]["Fmch_rate_val"];
				orderpaybill.product_rate_val = resMVector[i]["Fproduct_rate_val"];
				orderpaybill.serv_rate_val = resMVector[i]["Fplatform_rate_val"];
				orderpaybill.order_status = resMVector[i]["Forder_status"];

				orderPayBillMap.insert(std::make_pair(orderpaybill.order_no, orderpaybill));
			}
			else if(resMVector[i]["Forder_status"] == ORDER_REFUND)
			{
				OrderRefundBillSummary orderrefundbill;
				orderrefundbill.Reset();
				orderrefundbill.order_no = resMVector[i]["Forder_no"];
				orderrefundbill.out_order_no = resMVector[i]["Fout_order_no"];
				orderrefundbill.agent_id = resMVector[i]["Fagent_id"];
				orderrefundbill.mch_id = resMVector[i]["Fmch_id"];
				orderrefundbill.pay_channel = resMVector[i]["Fpay_channel_id"];
				orderrefundbill.transaction_id = resMVector[i]["Ftransaction_id"];
				orderrefundbill.refund_amount = resMVector[i]["Frefund_amount"];
				orderrefundbill.order_status = resMVector[i]["Forder_status"];

				orderRefundBillMap.insert(std::make_pair(orderrefundbill.order_no, orderrefundbill));
			}

		}
	}

	CDEBUG_LOG("order paybill num :[%d], order refundbill num:[%d]",orderPayBillMap.size(),orderRefundBillMap.size());

	//查银行对账记录
	if(m_InParams["channel"] == "SWIFT")
	{
		//TODO:
		resMVector.clear();
		sqlss.str("");
		sqlss <<"SELECT Fpay_time,Ftrade_type,Fmch_order_no,Ftotal_amount*100 as Ftotal_amount,Ftotal_fee*100 as Ftotal_fee,Fout_order_no,Forder_status"
			  <<" FROM "<<ROUTE_BILL_DB<<"."<<SWIFT_CHECK_BILL
			  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"';";

		iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),resMVector);
		if(iRet == 1)  //有记录
		{
			for(size_t i = 0; i < resMVector.size(); i++)
			{
				if(resMVector[i]["Forder_status"] == "转入退款")   //退货，退款交易
				{
					BankpayBillSummary bankrefundbill;
					bankrefundbill.Reset();
					bankrefundbill.bill_date = toDateEx(resMVector[i]["Fpay_time"]);
					bankrefundbill.order_no = resMVector[i]["Fmch_order_no"];
					bankrefundbill.pay_time = resMVector[i]["Fpay_time"];
					bankrefundbill.pay_amount = resMVector[i]["Ftotal_amount"];
					bankrefundbill.pay_fee = resMVector[i]["Ftotal_fee"];
					bankrefundbill.ori_order_no = resMVector[i]["Fout_order_no"];

					bankRefundBillMap.insert(std::make_pair(bankrefundbill.order_no, bankrefundbill));

				}
				else //否则就是正常交易
				{
					BankpayBillSummary bankpaybill;
					bankpaybill.Reset();
					bankpaybill.bill_date = toDateEx(resMVector[i]["Fpay_time"]);
					bankpaybill.order_no = resMVector[i]["Fmch_order_no"];  //商户订单号，对应本地平台订单号
					bankpaybill.pay_time = resMVector[i]["Fpay_time"];
					bankpaybill.pay_amount = resMVector[i]["Ftotal_amount"];
					bankpaybill.pay_fee = resMVector[i]["Ftotal_fee"];
					bankpaybill.ori_order_no = resMVector[i]["Fout_order_no"];

					bankPayBillMap.insert(std::make_pair(bankpaybill.order_no, bankpaybill));
				}

			}
		}
	}
	else if(m_InParams["channel"] == "SPEEDPOS")
	{
		//TODO:
		resMVector.clear();
		sqlss.str("");
		sqlss <<"SELECT Fpay_time,Forder_no,Fout_order_no,Fmch_id,Ftrade_type,Forder_status,Fpayment_fee,Ffee,Frefund_fee,Frefund_no,Fout_refund_no"
			  <<" FROM "<<ROUTE_BILL_DB<<"."<<SPEEDPOS_CHENK_BILL
			  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"';";

		iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),resMVector);
		if(iRet == 1)  //有记录
		{
			for(size_t i = 0; i < resMVector.size(); i++)
			{
				if(STOI(resMVector[i]["Frefund_fee"]) > 0)   //退货，退款交易
				{
					BankpayBillSummary bankrefundbill;
					bankrefundbill.Reset();
					bankrefundbill.bill_date = toDateEx(resMVector[i]["Fpay_time"]);
					bankrefundbill.order_no = resMVector[i]["Forder_no"];
					bankrefundbill.pay_time = resMVector[i]["Fpay_time"];
					bankrefundbill.pay_amount = resMVector[i]["Fpayment_fee"];
					bankrefundbill.pay_fee = resMVector[i]["Ffee"];
					bankrefundbill.ori_order_no = resMVector[i]["Fout_order_no"];

					bankRefundBillMap.insert(std::make_pair(bankrefundbill.order_no, bankrefundbill));

				}
				else //否则就是正常交易
				{
					BankpayBillSummary bankpaybill;
					bankpaybill.Reset();
					bankpaybill.bill_date = toDateEx(resMVector[i]["Fpay_time"]);
					bankpaybill.order_no = resMVector[i]["Forder_no"];
					bankpaybill.pay_time = resMVector[i]["Fpay_time"];
					bankpaybill.pay_amount = resMVector[i]["Fpayment_fee"];
					bankpaybill.pay_fee = resMVector[i]["Ffee"];
					bankpaybill.ori_order_no = resMVector[i]["Fout_order_no"];

					bankPayBillMap.insert(std::make_pair(bankpaybill.order_no, bankpaybill));
				}
			}
		}

	}
	else  //深浦发
	{
		resMVector.clear();
		sqlss.str("");
		sqlss <<"SELECT Fbill_date,Ftrade_type,Forder_no,Fpay_time,Fpay_amount,Fpay_fee,Fori_order_no"
			  <<" FROM "<<ROUTE_BILL_DB<<"."<<SZSPDB_CHECK_BILL
			  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"';";

		iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),resMVector);
		if(iRet == 1)  //有记录
		{
			for(size_t i = 0; i < resMVector.size(); i++)
			{
				if(resMVector[i]["Ftrade_type"] == "02")   //退货，退款交易
				{
					BankpayBillSummary bankrefundbill;
					bankrefundbill.Reset();
					bankrefundbill.bill_date = resMVector[i]["Fbill_date"];
					bankrefundbill.order_no = resMVector[i]["Forder_no"];
					bankrefundbill.pay_time = resMVector[i]["Fpay_time"];
					bankrefundbill.pay_amount = resMVector[i]["Fpay_amount"];
					bankrefundbill.pay_fee = resMVector[i]["Fpay_fee"];
					bankrefundbill.ori_order_no = resMVector[i]["Fori_order_no"];

					bankRefundBillMap.insert(std::make_pair(bankrefundbill.order_no, bankrefundbill));

				}
				else //否则就是正常交易
				{
					BankpayBillSummary bankpaybill;
					bankpaybill.Reset();
					bankpaybill.bill_date = resMVector[i]["Fbill_date"];
					bankpaybill.order_no = resMVector[i]["Forder_no"];
					bankpaybill.pay_time = resMVector[i]["Fpay_time"];
					bankpaybill.pay_amount = resMVector[i]["Fpay_amount"];
					bankpaybill.pay_fee = resMVector[i]["Fpay_fee"];
					bankpaybill.ori_order_no = resMVector[i]["Fori_order_no"];

					bankPayBillMap.insert(std::make_pair(bankpaybill.order_no, bankpaybill));
				}

			}
		}
	}

	CDEBUG_LOG("bank paybill num :[%d], bank refundbill num:[%d]",bankPayBillMap.size(),bankRefundBillMap.size());

}

void CBillCheckTask::CompareSuccess()
{
	m_abnor_num = 0;
	m_abnor_amount = 0;

	for(auto iter : orderPayBillMap)
	{
		auto bank_iter = bankPayBillMap.find(iter.first);
		if(bank_iter == bankPayBillMap.end())  //银行无数据
		{
			CDEBUG_LOG("order pay overflow :[%s],----NOT Match----",iter.second.order_no.c_str());
			++m_abnor_num;
			m_abnor_amount += atol(iter.second.total_amount.c_str());
			CDEBUG_LOG("===1========NOT Match :m_abnor_num =[%d],m_abnor_amount = [%ld]",m_abnor_num,m_abnor_amount);
			InsertAbnormalDB(ORDER_SUCCESS,ABNORMAL_ORDER_OVER_FLOW,iter.second.order_no,iter.second.total_amount);  //本地多
		}
		else //有数据
		{
			//状态两边都是成功的，只对金额
			if(atol(iter.second.total_amount.c_str()) != atol((*bank_iter).second.pay_amount.c_str()))
			{
				CDEBUG_LOG("order pay amount not match ,order :[%s],bank :[%s],----NOT Match----",
						iter.second.total_amount.c_str(),(*bank_iter).second.pay_amount.c_str());
				++m_abnor_num;
				m_abnor_amount += atol(iter.second.total_amount.c_str());   //算平台的
				CDEBUG_LOG("==2=========NOT Match :m_abnor_num =[%d],m_abnor_amount = [%ld]",m_abnor_num,m_abnor_amount);
				InsertAbnormalDB(ORDER_SUCCESS,ABNORMAL_AMOUNT_NOT_MATCH,iter.second.order_no,iter.second.total_amount,(*bank_iter).second.order_no,(*bank_iter).second.pay_amount);  //金额不一致
			}
			else  //平账，更新对账状态
			{
				CDEBUG_LOG("order pay  :[%s], ====Match====",iter.second.order_no.c_str());
				UpdateCheckBillDB(iter.second.order_no);
			}
			//有数据，删除银行方流水
			bankPayBillMap.erase(bank_iter++);

		}
	}

	//剩下的银行方如果有数据 ，那就是银行方多
	for(auto bankiter:bankPayBillMap)
	{
		CDEBUG_LOG("bank pay :[%s], ----NOT Match----",bankiter.second.order_no.c_str());

		int iRet = BankOverflowQuery(bankiter.second.order_no,1);
		if(iRet < 0)  //无记录
		{
			InsertAbnormalDB(ORDER_SUCCESS,ABNORMAL_BANK_OVER_FLOW,"","",bankiter.second.order_no,bankiter.second.pay_amount);
		}
		else  //有记录
		{
			InsertAbnormalDB(ORDER_SUCCESS,ABNORMAL_STATE_NOT_MATCH,"","",bankiter.second.order_no,bankiter.second.pay_amount);
		}

		++m_abnor_num;
		m_abnor_amount += atol(bankiter.second.pay_amount.c_str());
		CDEBUG_LOG("==3=========NOT Match :m_abnor_num =[%d],m_abnor_amount = [%ld]",m_abnor_num,m_abnor_amount);
	}


}

void CBillCheckTask::CompareRefund()
{
	for(auto iter : orderRefundBillMap)
	{
		auto bank_iter = bankRefundBillMap.find(iter.first);
		if(bank_iter == bankRefundBillMap.end())  //银行无数据
		{
			CDEBUG_LOG("order refund overflow :[%s],----NOT Match----",iter.second.order_no.c_str());
			++m_abnor_num;
			m_abnor_amount += atol(iter.second.refund_amount.c_str());
			CDEBUG_LOG("==4=========NOT Match :m_abnor_num =[%d],m_abnor_amount = [%ld]",m_abnor_num,m_abnor_amount);
			InsertAbnormalDB(ORDER_REFUND,ABNORMAL_ORDER_OVER_FLOW,iter.second.order_no,iter.second.refund_amount);  //本地多
		}
		else //有数据
		{
			//状态两边都是成功的，只对金额
			if(atol(iter.second.refund_amount.c_str()) != atol((*bank_iter).second.pay_amount.c_str()))
			{
				CDEBUG_LOG("order refund amount not match ,order :[%s],bank :[%s],----NOT Match----",
						iter.second.refund_amount.c_str(),(*bank_iter).second.pay_amount.c_str());
				++m_abnor_num;
				m_abnor_amount += atol(iter.second.refund_amount.c_str());   //算平台的
				CDEBUG_LOG("==5=========NOT Match :m_abnor_num =[%d],m_abnor_amount = [%ld]",m_abnor_num,m_abnor_amount);
				InsertAbnormalDB(ORDER_REFUND,ABNORMAL_AMOUNT_NOT_MATCH,iter.second.order_no,iter.second.refund_amount,(*bank_iter).second.order_no,(*bank_iter).second.pay_amount);  //金额不一致
			}
			else  //平账，更新对账状态
			{
				CDEBUG_LOG("order refund  :[%s], ====Match====",iter.second.order_no.c_str());
				UpdateCheckBillDB(iter.second.order_no);
			}
			//有数据，删除银行方流水
			bankRefundBillMap.erase(bank_iter++);

		}
	}

	//剩下的银行方如果有数据 ，那就是银行方多
	for(auto bankiter:bankRefundBillMap)
	{
		CDEBUG_LOG("bank refund :[%s], ----NOT Match----",bankiter.second.order_no.c_str());

		//先查询一下交易表，再确认是不是真的多
		int iRet = BankOverflowQuery(bankiter.second.order_no,2);
		if(iRet < 0)  //无记录
		{
			InsertAbnormalDB(ORDER_REFUND,ABNORMAL_BANK_OVER_FLOW,"","",bankiter.second.order_no,bankiter.second.pay_amount);
		}
		else  //有记录
		{
			InsertAbnormalDB(ORDER_REFUND,ABNORMAL_STATE_NOT_MATCH,"","",bankiter.second.order_no,bankiter.second.pay_amount);
		}
		++m_abnor_num;
		m_abnor_amount += atol(bankiter.second.pay_amount.c_str());
		CDEBUG_LOG("==6=========NOT Match :m_abnor_num =[%d],m_abnor_amount = [%ld]",m_abnor_num,m_abnor_amount);
		   //银行方多
	}
}

void CBillCheckTask::InsertAbnormalDB(const string& bill_type,int ab_type,const string& order_no, const string& order_amt ,const string& bank_no, const string& bank_amt)
{
	int iRet = 0;

	string str_order_amt = order_amt.empty() ? "0" :order_amt;
	string str_bank_amt = bank_amt.empty() ? "0" :bank_amt;

	sqlss.str("");
	sqlss <<"INSERT INTO "<<ROUTE_BILL_DB<<"."<<BILL_ABNORMAL
		  <<" (Fbill_date,Fpay_channel,Fpf_trade_amount,Fpf_order_no,Fch_trade_amount,Fch_order_no,Forder_status,"
		  <<"Fabnormal_type,Fporcess_status) values('"
		  <<m_InParams["bill_date"]<<"','"<<m_InParams["pay_channel"]<<"','"<<str_order_amt<<"','"<<order_no
		  <<"',"<<str_bank_amt<<",'"<<bank_no<<"','"<<bill_type<<"',"<<ab_type<<",0)";

    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
    if(iRet != 1)
    {
    	CERROR_LOG("insert t_route_bill_abnormal fail!!!");
    	throw(CTrsExp(INSERT_DB_ERR,"insert t_route_bill_abnormal fail!!!"));
    }
}

void CBillCheckTask::UpdateCheckBillDB(const string& order_no)
{
	int iRet = 0;

	sqlss.str("");
	//更新为平账
	sqlss <<" UPDATE "<<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
		  <<" SET Fcheck_status = "<<CHECK_BILL_SUCC<<" where Forder_no = '"<<order_no<<"';";

    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
    if(iRet != 1)
    {
    	CERROR_LOG("insert t_route_check_bill fail!!!");
    	throw(CTrsExp(UPDATE_DB_ERR,"insert t_route_check_bill fail!!!"));
    }
}

INT32 CBillCheckTask::BankOverflowQuery(string& order_no,int order_flag)
{
	//int iRet = 0;
	char szRecvBuff[10240] = {0};
	StringMap paramMap;
	StringMap recvMap;
	CSocket* tradeSocket = Singleton<CSpeedPosConfig>::GetInstance()->GetTradeServerSocket();

	if(order_flag == 1)  //交易
	{
		paramMap.insert(StringMap::value_type("cmd","1007"));
		paramMap.insert(StringMap::value_type("ver","1.0"));

		JsonMap  bizCJsonMap;
		bizCJsonMap.insert(JsonMap::value_type(JsonType("order_no"), order_no));
		std::string bizContent = JsonUtil::objectToString(bizCJsonMap);
		paramMap.insert(std::make_pair("biz_content", bizContent));
	}
	else
	{
		paramMap.insert(StringMap::value_type("cmd","1009"));
		paramMap.insert(StringMap::value_type("ver","1.0"));

		JsonMap  bizCJsonMap;
		bizCJsonMap.insert(JsonMap::value_type(JsonType("refund_no"), order_no));
		std::string bizContent = JsonUtil::objectToString(bizCJsonMap);
		paramMap.insert(std::make_pair("biz_content", bizContent));
	}


	tradeSocket->SendAndRecvLineEx(paramMap,szRecvBuff,sizeof(szRecvBuff),"\r\n");
	CDEBUG_LOG("recv Msg 111111111[%s]",szRecvBuff);

	if(NULL == szRecvBuff||strlen(szRecvBuff) == 0)  //
	{
		CDEBUG_LOG("query bank overflow order fail !!,err_msg[%s]",recvMap["retmsg"].c_str());
		throw(CTrsExp(ERR_QUERY_ORDER_STATE,"query bank overflow order fail!!"));
	}

	cJSON* root = cJSON_Parse(szRecvBuff);
	if (0 == strcmp(cJSON_GetObjectItem(root, "ret_code")->valuestring, "DD200210"))
	{
		CERROR_LOG("ret_code:[%s] ret_msg:[%s]\n", cJSON_GetObjectItem(root, "ret_code")->valuestring, cJSON_GetObjectItem(root, "ret_msg")->valuestring);
		return -1;
	}
	else if(0 == strcmp(cJSON_GetObjectItem(root, "ret_code")->valuestring, "0"))
	{
		return 0;
	}
	else
	{
		CERROR_LOG("query order server error :ret_code:[%s] ret_msg:[%s]\n", cJSON_GetObjectItem(root, "ret_code")->valuestring, cJSON_GetObjectItem(root, "ret_msg")->valuestring);
		throw(CTrsExp(ERR_CALL_ORDER_SERV,"查询订单服务失败！"));
	}

	//CDEBUG_LOG("CallPayGate2DownCheckBill,retcode=[%s]",recvMap["retcode"].c_str());

	//return -2;

}

void CBillCheckTask::InsertIntoSummary()
{
	int iRet = 0;
	int check_bill_result = -1;
	SqlResultSet  OutMap;
	SqlResultSet  SummaryMap;
	long total_fee;
	long refund_fee;

	if(m_abnor_num != 0 ||m_abnor_amount != 0)
	{
		check_bill_result = 2;  //不平
	}
	else
	{
		check_bill_result = 1;
	}

	//本地成功
	sqlss.str("");
	sqlss <<"SELECT Fpay_channel,count(*) as pf_total_count ,sum(Ftotal_amount) as pf_total_amt, sum(Ffactor_rate_val) as factor_fee,"
		  <<"sum(Fmch_rate_val) as mch_fee,sum(Fproduct_rate_val) as product_fee,sum(Fplatform_rate_val) as serv_rate_val from "<<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
		  <<" where Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' and Fpay_channel_id = '"<<m_InParams["pay_channel"]
		  <<"' and Forder_status = '"<<ORDER_SUCCESS<<"';";


	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
	SummaryMap["channel_name"] = OutMap["Fpay_channel"].empty() ? "":OutMap["Fpay_channel"];
	SummaryMap["pf_total_count"] = OutMap["pf_total_count"].empty() ? "0":OutMap["pf_total_count"];
	SummaryMap["pf_total_amt"] = OutMap["pf_total_amt"].empty() ? "0":OutMap["pf_total_amt"];
	SummaryMap["factor_fee"] = OutMap["factor_fee"].empty() ? "0":OutMap["factor_fee"];
	SummaryMap["mch_fee"] = OutMap["mch_fee"].empty() ? "0":OutMap["mch_fee"];
	SummaryMap["product_fee"] = OutMap["product_fee"].empty() ? "0":OutMap["product_fee"];
	SummaryMap["serv_rate_val"] = OutMap["serv_rate_val"].empty() ? "0":OutMap["serv_rate_val"];

	//total_fee = atol(SummaryMap["factor_fee"].c_str()) + atol(SummaryMap["mch_fee"].c_str()) + atol(SummaryMap["product_fee"].c_str());
	total_fee = atol(SummaryMap["mch_fee"].c_str());

	CDEBUG_LOG("------>pf_total_count = [%s],total_amount = [%s],total_fee = [%ld]",SummaryMap["pf_total_count"].c_str(),SummaryMap["pf_total_amt"].c_str(),total_fee);

	//本地退款
	sqlss.str("");
	OutMap.clear();
	sqlss <<"SELECT count(*) as pf_refund_count ,sum(Frefund_amount) as pf_refund_amt, sum(Ffactor_rate_val) as factor_fee,"
		  <<"sum(Fmch_rate_val) as mch_fee,sum(Fproduct_rate_val) as product_fee,sum(Fplatform_rate_val) as serv_rate_val from "<<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
		  <<" where Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' and Fpay_channel_id = '"<<m_InParams["pay_channel"]
		  <<"' and Forder_status = '"<<ORDER_REFUND<<"';";

	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
	SummaryMap["pf_refund_count"] = OutMap["pf_refund_count"].empty() ? "0":OutMap["pf_refund_count"];
	SummaryMap["pf_refund_amt"] = OutMap["pf_refund_amt"].empty() ? "0":OutMap["pf_refund_amt"];
	SummaryMap["factor_fee"] = OutMap["factor_fee"].empty() ? "0":OutMap["factor_fee"];
	SummaryMap["mch_fee"] = OutMap["mch_fee"].empty() ? "0":OutMap["mch_fee"];
	SummaryMap["product_fee"] = OutMap["product_fee"].empty() ? "0" :OutMap["product_fee"];
	SummaryMap["serv_rate_val"] = OutMap["serv_rate_val"].empty() ? "0" :OutMap["serv_rate_val"];

	//refund_fee = atol(SummaryMap["factor_fee"].c_str()) + atol(SummaryMap["mch_fee"].c_str()) + atol(SummaryMap["product_fee"].c_str());
	refund_fee = atol(SummaryMap["mch_fee"].c_str());

	CDEBUG_LOG("----->pf_refund_count = [%s],pf_refund_amount = [%s],total_fee = [%ld]",SummaryMap["pf_refund_count"].c_str(),SummaryMap["pf_refund_amt"].c_str(),refund_fee);


	if(m_InParams["channel"] == "SWIFT")
	{
		//TODO:
		//银行成功
		sqlss.str("");
		OutMap.clear();
		sqlss <<"SELECT count(*) as ch_total_count,sum(Ftotal_amount*100) as ch_total_amt, sum(Ftotal_fee*100) as ch_total_fee "
			  <<" FROM "<<ROUTE_BILL_DB<<"."<<SWIFT_CHECK_BILL
			  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' and Forder_status = '支付成功';";

		iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
		SummaryMap["ch_total_count"] = OutMap["ch_total_count"].empty() ?"0":OutMap["ch_total_count"];
		SummaryMap["ch_total_amt"] = OutMap["ch_total_amt"].empty() ?"0":OutMap["ch_total_amt"];
		SummaryMap["ch_total_fee"] = OutMap["ch_total_fee"].empty() ?"0":OutMap["ch_total_fee"];

		CDEBUG_LOG("------>bank total_count = [%s],total_amt = [%s]",SummaryMap["ch_total_count"].c_str(),SummaryMap["ch_total_amt"].c_str());

		//银行退款
		sqlss.str("");
		OutMap.clear();
		sqlss <<"SELECT count(*) as ch_refund_count,sum(Frefund_amount*100) as ch_refund_amt, sum(Ftotal_fee*100) as ch_refund_fee "
			  <<" FROM "<<ROUTE_BILL_DB<<"."<<SWIFT_CHECK_BILL
			  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' and Forder_status = '转入退款';";

		iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
		SummaryMap["ch_refund_count"] = OutMap["ch_refund_count"].empty() ?"0":OutMap["ch_refund_count"];
		SummaryMap["ch_refund_amt"] = OutMap["ch_refund_amt"].empty() ?"0":ITOS(abs(atoi(OutMap["ch_refund_amt"].c_str())));
		SummaryMap["ch_refund_fee"] = OutMap["ch_refund_fee"].empty() ?"0":OutMap["ch_refund_fee"];
	}
	else if(m_InParams["channel"] == "SPEEDPOS")
	{
		//TODO:
		//银行成功
		sqlss.str("");
		OutMap.clear();
		sqlss <<"SELECT count(*) as ch_total_count,sum(Fpayment_fee) as ch_total_amt, sum(Ffee) as ch_total_fee "
			  <<" FROM "<<ROUTE_BILL_DB<<"."<<SPEEDPOS_CHENK_BILL
			  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' and Forder_status = 'SUCCESS';";

		iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
		SummaryMap["ch_total_count"] = OutMap["ch_total_count"].empty() ?"0":OutMap["ch_total_count"];
		SummaryMap["ch_total_amt"] = OutMap["ch_total_amt"].empty() ?"0":OutMap["ch_total_amt"];
		SummaryMap["ch_total_fee"] = OutMap["ch_total_fee"].empty() ?"0":OutMap["ch_total_fee"];

		CDEBUG_LOG("------>bank total_count = [%s],total_amt = [%s]",SummaryMap["ch_total_count"].c_str(),SummaryMap["ch_total_amt"].c_str());

		//银行退款
		sqlss.str("");
		OutMap.clear();
		sqlss <<"SELECT count(*) as ch_refund_count,sum(Frefund_fee) as ch_refund_amt, sum(Ffee) as ch_refund_fee "
			  <<" FROM "<<ROUTE_BILL_DB<<"."<<SPEEDPOS_CHENK_BILL
			  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' and Frefund_fee > 0;";

		iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
		SummaryMap["ch_refund_count"] = OutMap["ch_refund_count"].empty() ?"0":OutMap["ch_refund_count"];
		SummaryMap["ch_refund_amt"] = OutMap["ch_refund_amt"].empty() ?"0":ITOS(abs(atoi(OutMap["ch_refund_amt"].c_str())));
		SummaryMap["ch_refund_fee"] = OutMap["ch_refund_fee"].empty() ?"0":OutMap["ch_refund_fee"];

	}
	else
	{
		//银行成功
		sqlss.str("");
		OutMap.clear();
		sqlss <<"SELECT count(*) as ch_total_count,sum(Fpay_amount) as ch_total_amt, sum(Fpay_fee) as ch_total_fee "
			  <<" FROM "<<ROUTE_BILL_DB<<"."<<SZSPDB_CHECK_BILL
			  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' and Ftrade_type != '02';";  //交易类型

		iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
		SummaryMap["ch_total_count"] = OutMap["ch_total_count"].empty() ?"0":OutMap["ch_total_count"];
		SummaryMap["ch_total_amt"] = OutMap["ch_total_amt"].empty() ?"0":OutMap["ch_total_amt"];
		SummaryMap["ch_total_fee"] = OutMap["ch_total_fee"].empty() ?"0":OutMap["ch_total_fee"];

		CDEBUG_LOG("------>bank total_count = [%s],total_amt = [%s]",SummaryMap["ch_total_count"].c_str(),SummaryMap["ch_total_amt"].c_str());

		//银行退款
		sqlss.str("");
		OutMap.clear();
		sqlss <<"SELECT count(*) as ch_refund_count,sum(Fpay_amount) as ch_refund_amt, sum(Fpay_fee) as ch_refund_fee "
			  <<" FROM "<<ROUTE_BILL_DB<<"."<<SZSPDB_CHECK_BILL
			  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' and Ftrade_type = '02';";

		iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
		SummaryMap["ch_refund_count"] = OutMap["ch_refund_count"].empty() ?"0":OutMap["ch_refund_count"];
		SummaryMap["ch_refund_amt"] = OutMap["ch_refund_amt"].empty() ?"0":OutMap["ch_refund_amt"];
		SummaryMap["ch_refund_fee"] = OutMap["ch_refund_fee"].empty() ?"0":OutMap["ch_refund_fee"];
	}


	CDEBUG_LOG("------->bank refund_count = [%s],refund_amt =[%s]",SummaryMap["ch_refund_count"].c_str(),SummaryMap["ch_refund_amt"].c_str());

	sqlss.str("");
	sqlss <<"INSERT INTO "<<ROUTE_BILL_DB<<"."<<BILL_SUMMARY
		  <<" (Fbill_date,Fpay_channel,Fchanel_name,Fcur_type,Fpf_total_count,Fpf_total_amount,Fpf_total_refund_count,"
		  <<"Fpf_total_refund_amount,Fpf_total_fee,Fpf_refund_fee,Fch_total_count,Fch_total_amount,"
		  <<"Fch_total_refund_count,Fch_total_refund_amount,Fch_total_fee,Fch_refund_fee,"
		  <<"Fabnormal_count,Fabnormal_amount,Fbill_result) values('"
		  <<m_InParams["bill_date"]<<"','"<<m_InParams["pay_channel"]<<"','"<<SummaryMap["channel_name"]<<"','CNY',"<<SummaryMap["pf_total_count"]<<","<<SummaryMap["pf_total_amt"]
		  <<","<<SummaryMap["pf_refund_count"]<<","<<SummaryMap["pf_refund_amt"]<<","<<total_fee<<","<<refund_fee<<","<<SummaryMap["ch_total_count"]
		  <<","<<SummaryMap["ch_total_amt"]<<","<<SummaryMap["ch_refund_count"]<<","<<SummaryMap["ch_refund_amt"]<<","<<SummaryMap["ch_total_fee"]
		  <<","<<SummaryMap["ch_refund_fee"]<<","<<m_abnor_num<<","<<m_abnor_amount<<","<<check_bill_result<<");";

    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
    if(iRet != 1)
    {
    	CERROR_LOG("insert t_route_bill_summary fail!!!");
    	throw(CTrsExp(INSERT_DB_ERR,"insert t_route_bill_summary fail!!!"));
    }
}

void CBillCheckTask::SetRetParam()
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

void CBillCheckTask::BuildResp( CHAR** outbuf, INT32& outlen )
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

void CBillCheckTask::LogProcess()
{
}
