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
	sqlss <<"select Fbill_date,Fpay_channel from "<<ROUTE_BILL_DB<<"."<<BILL_SUMMARY
		  <<" where Fbill_date = '"<<m_InParams["bill_date"]<<"' and Fpay_channel = '"<<m_InParams["pay_channel"]
		  <<"';";
	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
	if(iRet == 1)
	{
		CERROR_LOG("distribution record exist :[%s],[%s]!",OutMap["Fbill_date"].c_str(),OutMap["Fpay_channel"].c_str());
		throw(CTrsExp(ERR_CHECK_BILL_EXIST,"当日已对账，不允许重复对账 !"));
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
	string strbill_file_name = pBillBusConfig->BusiConfig.m_spdb_bill_path +
			pBillBusConfig->BusiConfig.m_bill_file_prefix + "_" + m_InParams["bill_date"];
	CDEBUG_LOG("bill file name = [%s]",strbill_file_name.c_str());

	if(access(strbill_file_name.c_str(),F_OK) != 0)
	{
		CERROR_LOG("bill file not exist !",strbill_file_name.c_str());
		throw(CTrsExp(ERR_BILL_FILE_NOT_EXIST,"通道方对账文件不存在 !"));
	}

	//清空通道对账数据
	TracateBankDB();


	string strshell_name = pBillBusConfig->BusiConfig.m_filetodb_shell;
	CDEBUG_LOG("shell name = [%s]",strshell_name.c_str());

	char szLoadOrderFlowCmd[512] = { 0 };
	snprintf(szLoadOrderFlowCmd, sizeof(szLoadOrderFlowCmd), "sh %s %s %d %s %s %s %s %s",
			strshell_name.c_str(), m_pBillDB->ms_host, m_pBillDB->mi_port,
			m_pBillDB->ms_user, m_pBillDB->ms_pass, ROUTE_BILL_DB,strbill_file_name.c_str(),SZSPDB_CHECK_BILL);
	CDEBUG_LOG("szLoadOrderFlowCmd = [%s]",szLoadOrderFlowCmd);
	system(szLoadOrderFlowCmd);

}

void CBillCheckTask::TracateBankDB()
{
	int iRet = 0;

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
	resMVector.clear();
	sqlss.str("");
	sqlss <<"SELECT Fbill_date,Forder_no,Fpay_time,Fpay_amount,Fpay_fee,Fori_order_no"
		  <<" FROM "<<ROUTE_BILL_DB<<"."<<SZSPDB_CHECK_BILL
		  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"';";

	iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),resMVector);
	if(iRet == 1)  //有记录
	{
		for(size_t i = 0; i < resMVector.size(); i++)
		{
			if(resMVector[i]["Fori_order_no"].empty()) //先判断，如果退款单号为空，则是成功的交易
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
			else //否则是退款交易
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
	char szRecvBuff[1024] = {0};
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
	CDEBUG_LOG("recv Msg [%s]",szRecvBuff);

	if(NULL == szRecvBuff||strlen(szRecvBuff) == 0)  //
	{
		CDEBUG_LOG("query bank overflow order fail !!,err_msg[%s]",recvMap["retmsg"].c_str());
		throw(CTrsExp(ERR_QUERY_ORDER_STATE,"query bank overflow order fail!!"));
	}

	cJSON* root = cJSON_Parse(szRecvBuff);
	if (0 != strcmp(cJSON_GetObjectItem(root, "ret_code")->valuestring, "0"))
	{
		CERROR_LOG("ret_code:[%s] ret_msg:[%s]\n", cJSON_GetObjectItem(root, "ret_code")->valuestring, cJSON_GetObjectItem(root, "ret_msg")->valuestring);
		return -1;
	}


	CDEBUG_LOG("CallPayGate2DownCheckBill,retcode=[%s]",recvMap["retcode"].c_str());

	return 0;

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
		  <<" where Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' and Forder_status = '"<<ORDER_SUCCESS<<"';";

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

	CDEBUG_LOG("pf_total_count = [%s],total_amount = [%s],total_fee = [%ld]",SummaryMap["pf_total_count"].c_str(),SummaryMap["pf_total_amt"].c_str(),total_fee);

	//本地退款
	sqlss.str("");
	OutMap.clear();
	sqlss <<"SELECT count(*) as pf_refund_count ,sum(Frefund_amount) as pf_refund_amt, sum(Ffactor_rate_val) as factor_fee,"
		  <<"sum(Fmch_rate_val) as mch_fee,sum(Fproduct_rate_val) as product_fee,sum(Fplatform_rate_val) as serv_rate_val from "<<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
		  <<" where Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' and Forder_status = '"<<ORDER_REFUND<<"';";

	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
	SummaryMap["pf_refund_count"] = OutMap["pf_refund_count"].empty() ? "0":OutMap["pf_refund_count"];
	SummaryMap["pf_refund_amt"] = OutMap["pf_refund_amt"].empty() ? "0":OutMap["pf_refund_amt"];
	SummaryMap["factor_fee"] = OutMap["factor_fee"].empty() ? "0":OutMap["factor_fee"];
	SummaryMap["mch_fee"] = OutMap["mch_fee"].empty() ? "0":OutMap["mch_fee"];
	SummaryMap["product_fee"] = OutMap["product_fee"].empty() ? "0" :OutMap["product_fee"];
	SummaryMap["serv_rate_val"] = OutMap["serv_rate_val"].empty() ? "0" :OutMap["serv_rate_val"];

	//refund_fee = atol(SummaryMap["factor_fee"].c_str()) + atol(SummaryMap["mch_fee"].c_str()) + atol(SummaryMap["product_fee"].c_str());
	refund_fee = atol(SummaryMap["mch_fee"].c_str());

	CDEBUG_LOG("pf_refund_count = [%s],pf_refund_amount = [%s],total_fee = [%ld]",SummaryMap["pf_refund_count"].c_str(),SummaryMap["pf_refund_amt"].c_str(),refund_fee);

	//银行成功
	sqlss.str("");
	OutMap.clear();
	sqlss <<"SELECT count(*) as ch_total_count,sum(Fpay_amount) as ch_total_amt, sum(Fpay_fee) as ch_total_fee "
		  <<" FROM "<<ROUTE_BILL_DB<<"."<<SZSPDB_CHECK_BILL
		  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' and Fori_order_no = '';";

	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
	SummaryMap["ch_total_count"] = OutMap["ch_total_count"].empty() ?"0":OutMap["ch_total_count"];
	SummaryMap["ch_total_amt"] = OutMap["ch_total_amt"].empty() ?"0":OutMap["ch_total_amt"];
	SummaryMap["ch_total_fee"] = OutMap["ch_total_fee"].empty() ?"0":OutMap["ch_total_fee"];

	//银行退款
	sqlss.str("");
	OutMap.clear();
	sqlss <<"SELECT count(*) as ch_refund_count,sum(Fpay_amount) as ch_refund_amt, sum(Fpay_fee) as ch_refund_fee "
		  <<" FROM "<<ROUTE_BILL_DB<<"."<<SZSPDB_CHECK_BILL
		  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' and Fori_order_no != '';";

	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),OutMap);
	SummaryMap["ch_refund_count"] = OutMap["ch_refund_count"].empty() ?"0":OutMap["ch_refund_count"];
	SummaryMap["ch_refund_amt"] = OutMap["ch_refund_amt"].empty() ?"0":OutMap["ch_refund_amt"];
	SummaryMap["ch_refund_fee"] = OutMap["ch_refund_fee"].empty() ?"0":OutMap["ch_refund_fee"];

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
