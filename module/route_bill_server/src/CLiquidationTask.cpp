/*
 * CLiquidationTask.cpp
 *
 *  Created on: 2017年7月27日
 *      Author: hawrkchen
 */

#include "CLiquidationTask.h"

CLiquidationTask::CLiquidationTask()
{
	//ctor
}

CLiquidationTask::~CLiquidationTask()
{
	//dector
}


INT32 CLiquidationTask::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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

	    CheckBillResult();

	    GetLiquidationData();

	    GetChannelData();

	    ProcLiquidation();

		//SetRetParam();
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
void CLiquidationTask::FillReq( NameValueMap& mapInput)
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

void CLiquidationTask::CheckInput()
{

	if(m_InParams["ver"].empty() || m_InParams["ver"] != VER)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("bill_date", m_InParams["bill_date"], 1, 10, true);  //对账日期  20170710


	Check::CheckStrParam("pay_channel", m_InParams["pay_channel"], 1, 10, true);  //银行通道


}

INT32 CLiquidationTask::CalcEffectiveTimeBill()
{
	BEGIN_LOG(__func__);
//	if (m_InParams["bill_date"].empty())
//	{
//		m_InParams["bill_date"] = GetYesterDate();
//	}

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

//检查对账汇总表是否有数据，如没有，则不作清分
void CLiquidationTask::CheckBillResult()
{
	INT32 iRet = 0;
	SqlResultSet outMap;

	sqlss.str("");
	sqlss	<<" SELECT count(*) as count FROM "
				<< ROUTE_BILL_DB<<"."<<BILL_SUMMARY
				<<" WHERE Fbill_date = '"<<m_InParams["bill_date"]<<"' and Fpay_channel = '"<<m_InParams["pay_channel"]<<"';";

	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),outMap);
	if(iRet < 0)
	{
		CERROR_LOG("get t_route_bill_summary fail!!");
		throw(CTrsExp(QRY_DB_ERR,"get t_route_bill_summary fail!!"));
	}
	if(atoi(outMap["count"].c_str()) == 0)
	{
		CERROR_LOG("no check bill   data!!");
		throw(CTrsExp(ERR_CHECK_BILL_NOT_EXIST,"未生成当日对账记录，无法清算!!"));
	}

	//检查是否已生成清分记录
	sqlss.str("");
	sqlss	<<" SELECT count(*) as count FROM "
				<< ROUTE_BILL_DB<<"."<<BILL_DISTRIBUTION
				<<" WHERE Fbill_date = '"<<m_InParams["bill_date"]<<"' and Fpay_channel = '"<<m_InParams["pay_channel"]<<"';";

	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),outMap);
	if(iRet < 0)
	{
		CERROR_LOG("get t_route_bill_distribution fail!!");
		throw(CTrsExp(QRY_DB_ERR,"get t_route_bill_distribution fail!!"));
	}
	if(atoi(outMap["count"].c_str()) != 0)
	{
		CERROR_LOG("distribution data has exist!!");
		throw(CTrsExp(ERR_CHECK_BILL_NOT_EXIST,"当日清分记录已生成，不允许重复生成!!"));
	}
}

void CLiquidationTask::GetLiquidationData()
{
	CDEBUG_LOG("GetLiquidationData begin ................................");

	int iRet;
	SqlResultMapVector resMVector;

	//本地成功对账记录
	sqlss.str("");
	sqlss <<"SELECT Fmch_id,Fmch_name,Fpay_channel,count(*) as total_count,sum(Ftotal_amount) as Ftotal_amount,sum(Fmch_rate_val) as Fmch_rate_val,"
		  <<"sum(Ffactor_rate_val) as Ffactor_rate_val,sum(Fproduct_rate_val) as Fproduct_rate_val,sum(Fplatform_rate_val) as Fplatform_rate_val "
		  <<" FROM "<<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
		  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' AND Fpay_channel_id='"
		  <<m_InParams["pay_channel"]<<"' AND Forder_status = 'SUCCESS' AND Fcheck_status = 1 group by Fmch_id";

	iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),resMVector);
	if(iRet == 1)  //有记录
	{
		for(size_t i = 0; i < resMVector.size(); i++)
		{
			OrderPayLiquidate orderpayliq;
			orderpayliq.Reset();
			orderpayliq.mch_id = resMVector[i]["Fmch_id"];
			orderpayliq.mch_name = resMVector[i]["Fmch_name"];
			orderpayliq.pay_channel = resMVector[i]["Fpay_channel"];
			orderpayliq.total_count = atoi(resMVector[i]["total_count"].c_str());
			orderpayliq.total_amount = resMVector[i]["Ftotal_amount"] == "" ?0 :atol(resMVector[i]["Ftotal_amount"].c_str());
			orderpayliq.total_fee = resMVector[i]["Fmch_rate_val"] == "" ?0 :atol(resMVector[i]["Fmch_rate_val"].c_str());
			orderpayliq.agent_profit = resMVector[i]["Ffactor_rate_val"] == "" ?0 :atol(resMVector[i]["Ffactor_rate_val"].c_str());
			orderpayliq.bm_profit = resMVector[i]["Fproduct_rate_val"] == "" ?0 :atol(resMVector[i]["Fproduct_rate_val"].c_str());
			orderpayliq.serv_profit = resMVector[i]["Fplatform_rate_val"] == "" ?0 :atol(resMVector[i]["Fplatform_rate_val"].c_str());

			orderPayLiquiMap.insert(std::make_pair(orderpayliq.mch_id, orderpayliq));

		}
	}


	//本地退款
	sqlss.str("");
	sqlss <<"SELECT Fmch_id,Fmch_name,Fpay_channel,count(*) as refund_count,sum(Frefund_amount) as Frefund_amount,sum(Fmch_rate_val) as Fmch_rate_val,"
		  <<"sum(Ffactor_rate_val) as Ffactor_rate_val,sum(Fproduct_rate_val) as Fproduct_rate_val,sum(Fplatform_rate_val) as Fplatform_rate_val"
		  <<" FROM "<<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
		  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' AND Fpay_channel='"
		  <<m_InParams["pay_channel"]<<"' AND Forder_status = 'REFUND' AND Fcheck_status = 1 group by Fmch_id";

	iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),resMVector);
	if(iRet == 1)  //有记录
	{
		for(size_t i = 0; i < resMVector.size(); i++)
		{
			OrderPayLiquidate orderpayliq;
			orderpayliq.Reset();
			orderpayliq.mch_id = resMVector[i]["Fmch_id"];
			orderpayliq.mch_name = resMVector[i]["Fmch_name"];
			orderpayliq.pay_channel = resMVector[i]["Fpay_channel"];
			orderpayliq.refund_count = atoi(resMVector[i]["refund_count"].c_str());
			orderpayliq.refund_amount = resMVector[i]["Frefund_amount"] == "" ? 0 :atol(resMVector[i]["Frefund_amount"].c_str());
			orderpayliq.total_fee = resMVector[i]["Fmch_rate_val"] == "" ? 0 :atol(resMVector[i]["Fmch_rate_val"].c_str());
			orderpayliq.agent_profit = resMVector[i]["Ffactor_rate_val"] == "" ? 0 :atol(resMVector[i]["Ffactor_rate_val"].c_str());
			orderpayliq.bm_profit = resMVector[i]["Fproduct_rate_val"] == "" ? 0 :atol(resMVector[i]["Fproduct_rate_val"].c_str());
			orderpayliq.serv_profit = resMVector[i]["Fplatform_rate_val"] == "" ? 0 :atol(resMVector[i]["Fplatform_rate_val"].c_str());

			orderRefundLiquiMap.insert(std::make_pair(orderpayliq.mch_id, orderpayliq));


		}
	}

}

void CLiquidationTask::GetChannelData()
{
	CDEBUG_LOG("GetChannelData begin ................................");

	int iRet;
	SqlResultMapVector resMVector;

	//渠道表
	sqlss.str("");
	sqlss <<"SELECT Forder_no,Fmch_id,Fpay_channel_id,Fagent_id,count(*) as total_count,sum(Ftotal_amount) as total_amount,"
		  <<" sum(Fchannel_profit) as channel_profit FROM "<<ROUTE_BILL_DB<<"."<<T_ROUTE_CHANNEL
		  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' AND Fpay_channel_id='"
		  <<m_InParams["pay_channel"]<<"' AND Forder_status = 'SUCCESS' group by Fagent_id";
		//还要加上对账成功的条件

	iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),resMVector);
	if(iRet == 1)  //有记录
	{
		for(size_t i = 0; i < resMVector.size(); i++)
		{
			//校验是否是平账的记录
			string order_no =  resMVector[i]["Forder_no"];
			if(!CheckBillSuccess(order_no))  //对账不平的记录跳过
			{
				CDEBUG_LOG("order_no:[%s] check_status not success.",order_no.c_str());
				continue;
			}
			OrderPayLiquidate orderpayliq;
			orderpayliq.Reset();
			orderpayliq.mch_id = resMVector[i]["Fmch_id"];
			orderpayliq.pay_channel = resMVector[i]["Fpay_channel_id"];
			orderpayliq.agent_id = resMVector[i]["Fagent_id"];
			orderpayliq.total_count = atoi(resMVector[i]["total_count"].c_str());
			orderpayliq.total_amount = resMVector[i]["total_amount"] == "" ?0 :atol(resMVector[i]["total_amount"].c_str());
			orderpayliq.agent_profit = resMVector[i]["channel_profit"] == "" ? 0 :atol(resMVector[i]["channel_profit"].c_str());

			orderChannelLiquiMap.insert(std::make_pair(orderpayliq.agent_id, orderpayliq));

		}
	}

	//暂时不考虑渠道退款
}

bool CLiquidationTask::CheckBillSuccess(const string& order_no)
{
	INT32 iRet = 0;
	SqlResultSet outMap;

	sqlss.str("");
	sqlss	<<" SELECT Forder_no, Fcheck_status FROM "
				<< ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
				<<" WHERE Forder_no = '"<<order_no<<"';";

	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),outMap);
	if(iRet == 0)  //??无记录
	{
		CERROR_LOG("get t_route_check_bill record fail!!");
		throw(CTrsExp(QRY_DB_ERR,"get t_route_check_bill record fail!!"));
	}

	if(atoi(outMap["Fcheck_status"].c_str()) == CHECK_BILL_SUCC)
	{
		return true;
	}
	return false;
}

void CLiquidationTask::ProcLiquidation()
{
	CDEBUG_LOG("ProcLiquidation begin ................................");
	long bm_profit = 0;
	long serv_profit = 0;
	OrderPayLiquidate pay_liq;
	pay_liq.Reset();
	std::map<std::string, OrderPayLiquidate>::iterator itRefund;

	CDEBUG_LOG("order trade bill num:[%d],refund bill num:[%d],channel bill num:[%d]",
			orderPayLiquiMap.size(),orderRefundLiquiMap.size(),orderChannelLiquiMap.size());

	//算商户
	for (auto order_iter : orderPayLiquiMap)
	{
		pay_liq.Reset();
		pay_liq.mch_id = order_iter.first;
		pay_liq.mch_name = order_iter.second.mch_name;
		pay_liq.pay_channel = order_iter.second.pay_channel;
		pay_liq.total_count = order_iter.second.total_count;
		pay_liq.total_amount = order_iter.second.total_amount;
		pay_liq.total_fee = order_iter.second.total_fee;
		pay_liq.net_amount = pay_liq.total_amount - pay_liq.refund_amount;   //交易净额
		pay_liq.mch_net_amount = pay_liq.total_amount - pay_liq.total_fee;  //结算金额
		pay_liq.bm_profit = order_iter.second.bm_profit;
		pay_liq.serv_profit = order_iter.second.serv_profit;
		pay_liq.refund_amount = 0;

		if((itRefund = orderRefundLiquiMap.find(pay_liq.mch_id)) != orderRefundLiquiMap.end())
		{

			pay_liq.bm_profit = pay_liq.bm_profit - itRefund->second.bm_profit;  //通道成本
			pay_liq.serv_profit = pay_liq.serv_profit - itRefund->second.serv_profit;  //平台分润

			pay_liq.refund_count = itRefund->second.refund_count;   //退款笔数
			pay_liq.refund_amount = itRefund->second.refund_amount;   //退款金额
		}
		bm_profit += pay_liq.bm_profit;
		serv_profit += pay_liq.serv_profit;

		InsertLiquidationDB(FUND_TYPE_MCH,pay_liq);

	}

	//算平台收益
	pay_liq.serv_profit = serv_profit;
	pay_liq.mch_id = "";
	pay_liq.mch_name = "";
	InsertLiquidationDB(FUND_TYPE_SERV,pay_liq);

	//算渠道
	for (auto channel_iter : orderChannelLiquiMap)
	{
		pay_liq.Reset();
		pay_liq.agent_id = channel_iter.first;
		pay_liq.mch_id = channel_iter.second.mch_id;
		pay_liq.pay_channel = channel_iter.second.pay_channel;
		pay_liq.total_count = channel_iter.second.total_count;
		pay_liq.total_amount = channel_iter.second.total_amount;
		//pay_liq.total_fee = channel_iter.second.total_fee;
		pay_liq.mch_net_amount = channel_iter.second.agent_profit;
		pay_liq.serv_profit = channel_iter.second.agent_profit;

		InsertLiquidationDB(FUND_TYPE_CH,pay_liq);

	}

	//算银行成本
	//目前不算


}

void CLiquidationTask::InsertLiquidationDB(const string& fund_type,OrderPayLiquidate& liq_map)
{
	int iRet;
	sqlss.str("");
	sqlss <<"INSERT INTO "<<ROUTE_BILL_DB<<"."<<BILL_DISTRIBUTION
		  <<" (Fbill_date,Fpay_channel,Ffund_type,Fcur_type,Fmch_id,Fmch_name,"
		  <<"Ftrade_count,Ftrade_amount,Frefund_count,Frefund_amount,Fcost_fee,Fmch_fee,"
		  <<"Ftrade_net_amount,Fpend_settle,Fshare_profit) values('"
		  <<m_InParams["bill_date"]<<"','"<<m_InParams["pay_channel"]<<"','"<<fund_type<<"','CNY','"<<liq_map.mch_id
		  <<"','"<<liq_map.mch_name<<"',"<<liq_map.total_count<<","<<liq_map.total_amount<<","<<liq_map.refund_count
		  <<","<<liq_map.refund_amount<<","<<liq_map.bm_profit<<","<<liq_map.total_fee<<","<<liq_map.net_amount
		  <<","<<liq_map.mch_net_amount<<","<<liq_map.serv_profit<<");";

    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
    if(iRet != 1)
    {
    	CERROR_LOG("insert t_route_bill_distribution fail!!!");
    	throw(CTrsExp(INSERT_DB_ERR,"insert t_route_bill_distribution fail!!!"));
    }
}

void CLiquidationTask::SetRetParam()
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

void CLiquidationTask::BuildResp( CHAR** outbuf, INT32& outlen )
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

void CLiquidationTask::LogProcess()
{
}

