/*
 * CRouteBillSummaryQuery.cpp
 *
 *  Created on: 2017年7月20日
 *      Author: hawrkchen
 */

#include "CRouteBillSummaryQuery.h"

CRouteBillSummaryQuery::CRouteBillSummaryQuery()
{
   //ctor
}

CRouteBillSummaryQuery::~CRouteBillSummaryQuery()
{
	//dector
}


INT32 CRouteBillSummaryQuery::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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

		if(m_InParams["step"] == "1")   //STEP=1 页头的汇总数据
		{
			QuerySummaryCount();
		}
		else if(m_InParams["step"] == "2")  //STEP=2列表的汇总数据
		{
			QuerySummary();
		}
		else
		{
			CERROR_LOG(" invalid param [step],value [%s]!",m_InParams["step"].c_str());
			throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
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
void CRouteBillSummaryQuery::FillReq( NameValueMap& mapInput)
{
	NameValueMapIter iter;
	for(iter=mapInput.begin();iter!=mapInput.end();iter++)
	{
		string szName = iter->first;
		transform(szName.begin(), szName.end(), szName.begin(), ::tolower);
		//m_InParams[szName] = getSafeInput(iter->second);
		m_InParams[szName] = iter->second;
	}

	ParseJson2Map(m_InParams["biz_content"],m_InParams);


}

void CRouteBillSummaryQuery::CheckInput()
{

	if(m_InParams["ver"].empty() || m_InParams["ver"] != VER)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("input_time", m_InParams["input_time"], 1, 10);
	Check::CheckStrParam("end_time", m_InParams["end_time"], 1, 10);
	Check::CheckStrParam("pay_channel", m_InParams["pay_channel"], 1, 10);
	Check::CheckDigitalParam("bill_result", m_InParams["bill_result"], 1, 5);

	Check::CheckStrParam("step", m_InParams["step"], 1, 3,true);

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

void CRouteBillSummaryQuery::QuerySummaryCount()
{
	CDEBUG_LOG("-----begin--------");
	int iRet;
	ostringstream sWhereSql;
	SqlResultSet outMap;
	JsonList m_jsonList;

    if(!m_InParams["input_time"].empty()&& ! m_InParams["end_time"].empty())
    {
    	sWhereSql << "and Fbill_date between '" << m_InParams["input_time"] << "' and '" <<  m_InParams["end_time"] << "' ";
    }
    if(!m_InParams["pay_channel"].empty())
    {
    	sWhereSql << "and Fpay_channel = '" << m_InParams["pay_channel"] << "' ";
    }

    if(!m_InParams["bill_result"].empty())
    {
    	sWhereSql << "and Fbill_result = '"<< m_InParams["bill_result"]<<"' ";
    }

    sqlss.str("");
    sqlss <<"select sum(Fpf_total_amount-Fpf_total_refund_amount) as net_amount,"
    		"sum(Fpf_total_fee-Fpf_refund_fee) as net_fee,"
    		"sum(Fpf_total_amount-Fpf_total_refund_amount-Fpf_total_fee+Fpf_refund_fee) as net_pay_amount,"
    		"sum(Fpf_total_count) as total_count,"
    		"sum(Fpf_total_amount) as total_amount,"
    		"sum(Fpf_total_refund_count) as total_refund_count,"
    		"sum(Fpf_total_refund_amount) as total_refund_amount,"
    		"sum(Fabnormal_count) as abnormal_count,"
    		"sum(Fabnormal_amount) as abnormal_amount from "
    	  <<ROUTE_BILL_DB<<"."<<BILL_SUMMARY
		  <<" where 1=1 "
		  << sWhereSql.str()
		  <<";";

    iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),outMap);
    if(iRet == 1)  //有记录
    {
    	//m_jsonList.clear();
    	m_ContentJsonMap.clear();

    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("net_amount"), JsonType(outMap["net_amount"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("net_fee"), JsonType(outMap["net_fee"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("net_pay_amount"), JsonType(outMap["net_pay_amount"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total_count"), JsonType(outMap["total_count"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total_amount"), JsonType(outMap["total_amount"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total_refund_count"), JsonType(outMap["total_refund_count"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total_refund_amount"), JsonType(outMap["total_refund_amount"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("abnormal_count"), JsonType(outMap["abnormal_count"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("abnormal_amount"), JsonType(outMap["abnormal_amount"])));
		//m_jsonList.push_front(JsonList::value_type(BillJsonMap));

    }

	//m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType(m_jsonList)));

	CDEBUG_LOG("-----end--------");
}

void CRouteBillSummaryQuery::QuerySummary()
{
	CDEBUG_LOG("-----begin--------");
	int iRet;
	ostringstream sWhereSql;
	SqlResultSet outMap;
	SqlResultMapVector billInfoVector;
	JsonList m_jsonList;

	int iCnt      = (STOI(m_InParams["page"])-1)* STOI(m_InParams["limit"]);

    if(!m_InParams["input_time"].empty()&& ! m_InParams["end_time"].empty())
    {
    	sWhereSql << "and Fbill_date between '" << m_InParams["input_time"] << "' and '" <<  m_InParams["end_time"] << "' ";
    }
    if(!m_InParams["pay_channel"].empty())
    {
    	sWhereSql << "and Fpay_channel = '" << m_InParams["pay_channel"] << "' ";
    }
    if(!m_InParams["bill_result"].empty())
    {
    	sWhereSql << "and Fbill_result = '"<< m_InParams["bill_result"]<<"' ";
    }

    sqlss.str("");
    sqlss << "select count(*) as count from "
      	  <<ROUTE_BILL_DB<<"."<<BILL_SUMMARY
		  <<" where 1=1 "
  		  << sWhereSql.str()
		  <<";";
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
    sqlss <<"select Fbill_date,Fpay_channel,Fchanel_name,"
    		"Fpf_total_count,Fpf_total_amount,Fpf_total_refund_count,Fpf_total_refund_amount,Fpf_total_fee,Fpf_refund_fee,"
    		"Fch_total_count,Fch_total_amount,Fch_total_refund_count,Fch_total_refund_amount,Fch_total_fee,Fch_refund_fee,"
    		"Fabnormal_count,Fabnormal_amount,Fbill_result,Fcreate_time from "
    	  <<ROUTE_BILL_DB<<"."<<BILL_SUMMARY
		  <<" where  1=1 "
		  << sWhereSql.str()
		  << " limit " << iCnt << "," << m_InParams["limit"]  << ";";

    iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),billInfoVector);
    if(iRet == 1)  //有记录
    {
    	JsonMap BillJsonMap;
    	m_jsonList.clear();
		for(unsigned int i = 0; i < billInfoVector.size(); i++)
		{
			BillJsonMap.clear();
			BillJsonMap.insert(JsonMap::value_type(JsonType("bill_date"), JsonType(billInfoVector[i]["Fbill_date"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pay_channel"), JsonType(billInfoVector[i]["Fpay_channel"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("chanel_name"), JsonType(billInfoVector[i]["Fchanel_name"])));

			BillJsonMap.insert(JsonMap::value_type(JsonType("pf_total_count"), JsonType(billInfoVector[i]["Fpf_total_count"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pf_total_amount"), JsonType(billInfoVector[i]["Fpf_total_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pf_total_refund_count"), JsonType(billInfoVector[i]["Fpf_total_refund_count"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pf_total_refund_amount"), JsonType(billInfoVector[i]["Fpf_total_refund_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pf_total_fee"), JsonType(billInfoVector[i]["Fpf_total_fee"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pf_refund_fee"), JsonType(billInfoVector[i]["Fpf_refund_fee"])));

			BillJsonMap.insert(JsonMap::value_type(JsonType("ch_total_count"), JsonType(billInfoVector[i]["Fch_total_count"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("ch_total_amount"), JsonType(billInfoVector[i]["Fch_total_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("ch_total_refund_count"), JsonType(billInfoVector[i]["Fch_total_refund_count"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("ch_total_refund_amount"), JsonType(billInfoVector[i]["Fch_total_refund_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("ch_total_fee"), JsonType(billInfoVector[i]["Fch_total_fee"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("ch_refund_fee"), JsonType(billInfoVector[i]["Fch_refund_fee"])));

			BillJsonMap.insert(JsonMap::value_type(JsonType("abnormal_count"), JsonType(billInfoVector[i]["Fabnormal_count"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("abnormal_amount"), JsonType(billInfoVector[i]["Fabnormal_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("bill_result"), JsonType(billInfoVector[i]["Fbill_result"])));
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

	CDEBUG_LOG("-----end--------");
}


void CRouteBillSummaryQuery::SetRetParam()
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

void CRouteBillSummaryQuery::BuildResp( CHAR** outbuf, INT32& outlen )
{
	CDEBUG_LOG("Begin ...");

    CHAR szResp[ MAX_RESP_LEN ];
	JsonMap jsonRsp;

	if(m_RetMap["err_code"].empty())
	{
		m_RetMap["err_code"] = "0";
		m_RetMap["err_msg"]  = "success";
	}
	else
    {
        Singleton<CSpeedPosConfig>::GetInstance()->GetBusiConf()->ReadErrCodeFromFile(m_RetMap);
    }

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

void CRouteBillSummaryQuery::LogProcess()
{
}

