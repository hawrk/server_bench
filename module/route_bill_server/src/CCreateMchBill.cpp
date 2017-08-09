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
	vector<ChannelInfo> vec_channel;
	SqlResultMapVector resMVector;
	SqlResultMapVector mchMVector;
	//根据 当天对账日期 产生当天的对账单
	sqlss.str("");
	sqlss <<"SELECT distinct Fpay_channel_id  as pay_channel_id,Fpay_channel as channel_name"
		  <<" FROM "<<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
		  <<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"';";

	iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),resMVector);
	if(iRet == 0)  //无记录
	{
		CDEBUG_LOG("current date :[%s] no record...",m_InParams["bill_date"].c_str());
		return ;
	}

	for(size_t i = 0; i < resMVector.size(); i++)
	{
		ChannelInfo channelinfo;
		channelinfo.Reset();
		channelinfo.channel_id = resMVector[i]["pay_channel_id"];
		channelinfo.channel_name = resMVector[i]["channel_name"];
		vec_channel.push_back(channelinfo);
	}

	//
	for(auto channel :vec_channel)
	{
		mchPayBillMap.clear();
		mchRefundBillMap.clear();

		//商户成功
		mchMVector.clear();
		sqlss.str("");
		sqlss <<"SELECT Fmch_id,Fpay_channel_id,count(*) as total_count,sum(Ftotal_amount) as total_amount ,"
				"sum(Fmch_rate_val) as mch_rate_val FROM "
			  <<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
			  <<" where Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' AND Fpay_channel_id='"
			  << channel.channel_id<<"' AND Forder_status ='"<<ORDER_SUCCESS<<"' group by Fmch_id";

		iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),mchMVector);
		if(iRet == 1)
		{
			for(size_t i = 0; i < mchMVector.size(); i++)
			{
				MchBillSum mchbillsum;
				mchbillsum.Reset();
				mchbillsum.total_count = atoi(mchMVector[i]["total_count"].c_str());
				mchbillsum.total_amount = atol(mchMVector[i]["total_amount"].c_str());
				mchbillsum.total_fee =  atol(mchMVector[i]["mch_rate_val"].c_str());

				mchPayBillMap.insert(std::make_pair(mchMVector[i]["Fmch_id"], mchbillsum));
			}
		}

		//商户退款
		mchMVector.clear();
		sqlss.str("");
		sqlss <<"SELECT Fmch_id,count(*) as refund_count,sum(Frefund_amount) as refund_amount FROM "
			  <<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
			  <<" where Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' AND Fpay_channel_id='"
			  << channel.channel_id<<"' AND Forder_status ='"<<ORDER_REFUND<<"' group by Fmch_id";

		iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),mchMVector);
		if(iRet == 1)
		{
			for(size_t i = 0; i < mchMVector.size(); i++)
			{
				MchBillSum mchbillsum;
				mchbillsum.Reset();
				mchbillsum.refund_count = atoi(mchMVector[i]["refund_count"].c_str());
				mchbillsum.refund_amount = atol(mchMVector[i]["refund_amount"].c_str());

				mchRefundBillMap.insert(std::make_pair(mchMVector[i]["Fmch_id"], mchbillsum));
			}
		}

		//商户对账记录入库
		for(auto mch_iter: mchPayBillMap)
		{
			//std::map<std::string, MchBillSum>::iterator ref_iter;
			MchBillSum mchsum;
			mchsum.Reset();

			mchsum.mch_id = mch_iter.first;
			mchsum.total_amount = mch_iter.second.total_amount;
			mchsum.total_count = mch_iter.second.total_count;
			mchsum.net_amount = mch_iter.second.total_amount;
			mchsum.total_fee = mch_iter.second.total_fee;

			auto ref_iter = mchRefundBillMap.find(mch_iter.first);
			if(ref_iter != mchRefundBillMap.end())
			{
				mchsum.net_amount = mchsum.total_amount - ref_iter->second.refund_amount; //净额
				mchsum.refund_count = ref_iter->second.refund_count;
				mchsum.refund_amount = ref_iter->second.refund_amount;
			}

			InserIntoMchBill(channel,mchsum);
		}

	}

}

void CCreateMchBill::InserIntoMchBill(ChannelInfo& channel,MchBillSum& mch_bill)
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
		  <<" (Fmch_id,Fbill_date,Fpay_channel_id,Fpay_channel,Ftrade_count,Ftrade_amount,Frefund_count,"
		  <<"Frefund_amount,Fnet_amount,Ftotal_fee) values('"
		  <<mch_bill.mch_id<<"','"<<m_InParams["bill_date"]<<"','"<<channel.channel_id<<"','"<<channel.channel_name
		  <<"',"<<mch_bill.total_count<<","<<mch_bill.total_amount
		  <<","<<mch_bill.refund_count<<","<<mch_bill.refund_amount
		  <<","<<mch_bill.net_amount<<","<<mch_bill.total_fee<<");";

    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
    if(iRet != 1)
    {
    	CERROR_LOG("insert t_mch_check_bill fail!!!");
    	throw(CTrsExp(INSERT_DB_ERR,"insert t_mch_check_bill fail!!!"));
    }

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
