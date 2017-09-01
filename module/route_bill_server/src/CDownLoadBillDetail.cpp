/*
 * CDownLoadBillDetail.cpp
 *
 *  Created on: 2017年8月17日
 *      Author: hawrkchen
 */

#include "CDownLoadBillDetail.h"

CDownLoadBillDetail::CDownLoadBillDetail()
{
	//ctor
}

CDownLoadBillDetail::~CDownLoadBillDetail()
{
	//dector
}

INT32 CDownLoadBillDetail::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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

	    if(m_InParams["type"] == "1")
	    {
		    CreateMchBill();
	    }
	    else if(m_InParams["type"] == "2")
	    {
	    	CreateChanBill();
	    }
	    else
	    {
			CERROR_LOG(" invalid param [type],value [%s]!",m_InParams["type"].c_str());
			throw(CTrsExp(ERR_INVALID_PARAMS," type invalid!"));
	    }


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
void CDownLoadBillDetail::FillReq( NameValueMap& mapInput)
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

void CDownLoadBillDetail::CheckInput()
{

	if(m_InParams["ver"].empty() || m_InParams["ver"] != VER)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("bill_date", m_InParams["bill_date"], 1, 10, true);  //对账单日期
	Check::CheckDigitalParam("type", m_InParams["type"], 1, 3, true);  //下载类型 ：1 商户 2：渠道
	Check::CheckStrParam("fund_id", m_InParams["fund_id"], 1, 20, true);  //类型为商户时传商户号，类型为渠道时传渠道号

	Check::CheckPage(m_InParams["page"]);
	Check::CheckLimit(m_InParams["limit"]);

	if ( m_InParams["page"].empty() )
	{
		m_InParams["page"] = ITOS(PAGE_DEFAULT);
	}
	if ( m_InParams["limit"].empty() )
	{
		m_InParams["limit"] = ITOS(LIMIT_DEFAULT);
	}

}

INT32 CDownLoadBillDetail::CalcEffectiveTimeBill()
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


void CDownLoadBillDetail::CreateMchBill()
{
	int iRet;
	vector<ChannelInfo> vec_channel;
	SqlResultMapVector resMVector;
	JsonList m_jsonList;
	SqlResultSet outMap;

	int iCnt      = (STOI(m_InParams["page"])-1)* STOI(m_InParams["limit"]);

	sqlss.str("");
	sqlss <<"SELECT count(*) as count FROM "
		  <<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
		  <<" where Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' AND Fmch_id='"
		  << m_InParams["fund_id"]<<"' AND Fcheck_status = '1'";
    iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),outMap);

    if(outMap["count"] == "0")  //没有记录，直接返回
    {
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outMap["count"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("currentPage"), JsonType(m_InParams["page"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("totalPages"), JsonType(outMap["count"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("pageSize"), JsonType(m_InParams["limit"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType("")));
    	return ;
    }

	sqlss.str("");
	sqlss <<"SELECT Forder_no,Fout_order_no,Fpay_platform,Fpay_type,Fpay_channel_id,Ftotal_amount,Frefund_amount,Fmch_rate_val,"
			"Forder_status,Fpay_time FROM "
		  <<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
		  <<" where Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' AND Fmch_id='"
		  << m_InParams["fund_id"]<<"' AND Fcheck_status = '1'"
		  << " limit " << iCnt << "," << m_InParams["limit"]  << ";";

	iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),resMVector);
	if(iRet == 1)
	{
    	JsonMap BillJsonMap;
    	m_jsonList.clear();
		for(unsigned int i = 0; i < resMVector.size(); i++)
		{
			BillJsonMap.clear();
			BillJsonMap.insert(JsonMap::value_type(JsonType("order_no"), JsonType(resMVector[i]["Forder_no"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("out_order_no"), JsonType(resMVector[i]["Fout_order_no"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pay_platform"), JsonType(resMVector[i]["Fpay_platform"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pay_type"), JsonType(resMVector[i]["Fpay_type"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pay_channel_id"), JsonType(resMVector[i]["Fpay_channel_id"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("total_amount"), JsonType(resMVector[i]["Ftotal_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("refund_amount"), JsonType(resMVector[i]["Frefund_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("mch_rate_val"), JsonType(resMVector[i]["Fmch_rate_val"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("order_status"), JsonType(resMVector[i]["Forder_status"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pay_time"), JsonType(resMVector[i]["Fpay_time"])));

			m_jsonList.push_front(JsonList::value_type(BillJsonMap));
		}
	}
    m_ContentJsonMap.clear();
    int total_page =  ceil(STODOUBLE(outMap["count"])/STODOUBLE(m_InParams["limit"]));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outMap["count"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("totalPages"), JsonType(ITOS(total_page))));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("pageSize"), JsonType(m_InParams["limit"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("currentPage"), JsonType(m_InParams["page"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType(m_jsonList)));

}

void CDownLoadBillDetail::CreateChanBill()
{
	int iRet;
	vector<ChannelInfo> vec_channel;
	SqlResultMapVector resMVector;
	JsonList m_jsonList;
	SqlResultSet outMap;

	int iCnt      = (STOI(m_InParams["page"])-1)* STOI(m_InParams["limit"]);

	sqlss.str("");
	sqlss <<"SELECT count(*) as count FROM "
		  <<ROUTE_BILL_DB<<"."<<T_ROUTE_CHANNEL
		  <<" where Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"' AND Fagent_id='"
		  << m_InParams["fund_id"]<<"';";
    iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),outMap);

    if(outMap["count"] == "0")  //没有记录，直接返回
    {
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outMap["count"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("currentPage"), JsonType(m_InParams["page"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("totalPages"), JsonType(outMap["count"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("pageSize"), JsonType(m_InParams["limit"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType("")));
    	return ;
    }

	sqlss.str("");
	sqlss <<"SELECT chan.Forder_no as Forder_no,chan.Fpay_channel_id as Fpay_channel_id,chan.Fmch_id as Fmch_id,"
			"chan.Fpay_platform as Fpay_platform,chan.Fpay_type as Fpay_type,chan.Ftotal_amount as Ftotal_amount,"
			"chan.Fchannel_profit as Fchannel_profit,chan.Forder_status as Forder_status,chan.Fpay_time as Fpay_time FROM "
		  <<ROUTE_BILL_DB<<"."<<T_ROUTE_CHANNEL<<" chan inner join "<<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL<<" bill"
		  <<" ON chan.Forder_no = bill.Forder_no AND bill.Fcheck_status= 1 "
		  <<" AND chan.Fpay_time >='"<<m_start_time<<"' AND chan.Fpay_time <='"<<m_end_time<<"' AND chan.Fagent_id='"
		  << m_InParams["fund_id"]<<"'"
		  << " limit " << iCnt << "," << m_InParams["limit"]  << ";";

	iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),resMVector);
	if(iRet == 1)
	{
    	JsonMap BillJsonMap;
    	m_jsonList.clear();
		for(unsigned int i = 0; i < resMVector.size(); i++)
		{
			BillJsonMap.clear();
			BillJsonMap.insert(JsonMap::value_type(JsonType("order_no"), JsonType(resMVector[i]["Forder_no"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pay_channel_id"), JsonType(resMVector[i]["Fpay_channel_id"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("mch_id"), JsonType(resMVector[i]["Fmch_id"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pay_platform"), JsonType(resMVector[i]["Fpay_platform"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pay_type"), JsonType(resMVector[i]["Fpay_type"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("total_amount"), JsonType(resMVector[i]["Ftotal_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("channel_profit"), JsonType(resMVector[i]["Fchannel_profit"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("order_status"), JsonType(resMVector[i]["Forder_status"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pay_time"), JsonType(resMVector[i]["Fpay_time"])));

			m_jsonList.push_front(JsonList::value_type(BillJsonMap));
		}
	}
    m_ContentJsonMap.clear();
    int total_page =  ceil(STODOUBLE(outMap["count"])/STODOUBLE(m_InParams["limit"]));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outMap["count"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("totalPages"), JsonType(ITOS(total_page))));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("pageSize"), JsonType(m_InParams["limit"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("currentPage"), JsonType(m_InParams["page"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType(m_jsonList)));
}


void CDownLoadBillDetail::SetRetParam()
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

void CDownLoadBillDetail::BuildResp( CHAR** outbuf, INT32& outlen )
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

void CDownLoadBillDetail::LogProcess()
{
}
