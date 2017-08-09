/*
 * CRouteSettleQuery.cpp
 *
 *  Created on: 2017年7月20日
 *      Author: hawrkchen
 */
#include "CRouteSettleQuery.h"

CRouteSettleQuery::CRouteSettleQuery()
{
	//ctor
}
CRouteSettleQuery::~CRouteSettleQuery()
{
	//dector
}

INT32 CRouteSettleQuery::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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

	    if(m_InParams["type"] == "1")  //查商户
	    {
	    	QueryMchSettle();
	    }
	    else if(m_InParams["type"] == "2") //查询渠道
	    {
	    	QueryChannelSettle();
	    }
	    else if(m_InParams["type"] == "3")  //查平台
	    {
	    	QuerySerivSettle();
	    }
		else
		{
			CERROR_LOG(" invalid param [type],value [%s]!",m_InParams["type"].c_str());
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
void CRouteSettleQuery::FillReq( NameValueMap& mapInput)
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

void CRouteSettleQuery::CheckInput()
{

	if(m_InParams["ver"].empty() || m_InParams["ver"] != VER)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("input_time", m_InParams["input_time"], 1, 10);
	Check::CheckStrParam("end_time", m_InParams["end_time"], 1, 10);
	Check::CheckDigitalParam("type", m_InParams["type"], 1, 3,true);

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

void CRouteSettleQuery::QueryMchSettle()
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
    if(!m_InParams["bill_status"].empty())
    {
    	sWhereSql << "and Forder_status = '"<< m_InParams["bill_status"]<<"' ";
    }
    if(!m_InParams["proc_status"].empty())
    {
    	sWhereSql << "and Fporcess_status = '"<< m_InParams["proc_status"]<<"' ";
    }


    sqlss.str("");
    sqlss << "select count(*) as count from "
      	  <<ROUTE_BILL_DB<<"."<<BILL_ABNORMAL
		  <<" where 1=1 "
  		  << sWhereSql.str()
		  <<";";
    iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),outMap);

    if(outMap["count"] == "0")  //没有记录，直接返回
    {
        m_ContentJsonMap.clear();
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outMap["count"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("currentPage"), JsonType(m_InParams["page"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("totalPages"), JsonType(outMap["count"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("pageSize"), JsonType(m_InParams["limit"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType("")));
    	return ;
    }

    sqlss.str("");
    sqlss <<"select Fbill_date,Fpay_channel,"
    		"Fpf_trade_amount,Fpf_refund_amount,Fpf_order_no,Fpf_refund_no,"
    		"Fch_trade_amount,Fch_refund_amount,Fch_order_no,Fch_refund_no,"
    		"Ftrade_date,Forder_status,Fabnormal_type,Fporcess_status,Fcreate_time,Fmodify_time from "
    	  <<ROUTE_BILL_DB<<"."<<BILL_ABNORMAL
		  <<" where 1=1 "
		  << sWhereSql.str()
		  << " limit " << iCnt << "," << m_InParams["limit"] << ";";

    iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB,sqlss.str().c_str(),billInfoVector);
    if(iRet == 1)  //有记录
    {
    	JsonMap BillJsonMap;
    	m_jsonList.clear();
		for(unsigned int i = 0; i < billInfoVector.size(); i++)
		{
			BillJsonMap.clear();
			AddJsonMap(BillJsonMap,"bill_date",billInfoVector[i]["Fbill_date"]);
			AddJsonMap(BillJsonMap,"pay_channel",billInfoVector[i]["Fpay_channel"]);
			AddJsonMap(BillJsonMap,"pf_trade_amount",billInfoVector[i]["Fpf_trade_amount"]);
			AddJsonMap(BillJsonMap,"pf_refund_amount",billInfoVector[i]["Fpf_refund_amount"]);
			AddJsonMap(BillJsonMap,"pf_order_no",billInfoVector[i]["Fpf_order_no"]);
			AddJsonMap(BillJsonMap,"pf_refund_no",billInfoVector[i]["Fpf_refund_no"]);

			AddJsonMap(BillJsonMap,"ch_trade_amount",billInfoVector[i]["Fch_trade_amount"]);
			AddJsonMap(BillJsonMap,"ch_refund_amount",billInfoVector[i]["Fch_refund_amount"]);
			AddJsonMap(BillJsonMap,"ch_order_no",billInfoVector[i]["Fch_order_no"]);
			AddJsonMap(BillJsonMap,"ch_refund_no",billInfoVector[i]["Fch_refund_no"]);
			AddJsonMap(BillJsonMap,"trade_date",billInfoVector[i]["Ftrade_date"]);
			AddJsonMap(BillJsonMap,"abnormal_type",billInfoVector[i]["Fabnormal_type"]);
			AddJsonMap(BillJsonMap,"order_status",billInfoVector[i]["Forder_status"]);
			AddJsonMap(BillJsonMap,"porcess_status",billInfoVector[i]["Fporcess_status"]);
			AddJsonMap(BillJsonMap,"create_time",billInfoVector[i]["Fcreate_time"]);
			AddJsonMap(BillJsonMap,"modify_time",billInfoVector[i]["Fmodify_time"]);

			m_jsonList.push_front(JsonList::value_type(BillJsonMap));
		}

    }
    m_ContentJsonMap.clear();
    int total_page =  ceil(STOI(outMap["count"])/STOI(m_InParams["limit"]));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outMap["count"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("totalPages"), JsonType(ITOS(total_page))));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("pageSize"), JsonType(m_InParams["limit"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("currentPage"), JsonType(m_InParams["page"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType(m_jsonList)));

	CDEBUG_LOG("-----end--------");
}

void CRouteSettleQuery::QueryChannelSettle()
{

}

void CRouteSettleQuery::QuerySerivSettle()
{

}


void CRouteSettleQuery::SetRetParam()
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

void CRouteSettleQuery::BuildResp( CHAR** outbuf, INT32& outlen )
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

void CRouteSettleQuery::LogProcess()
{
}


