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

	    if(!m_InParams["fund_type"].empty())
	    {
	    	QueryLIquidation();
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
	Check::CheckStrParam("fund_id", m_InParams["fund_id"], 1, 10);
	Check::CheckStrParam("fund_name", m_InParams["fund_name"], 1, 64);
	Check::CheckStrParam("account_status", m_InParams["account_status"], 1, 10);

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

	if(m_InParams["type"] == "1")
	{
		m_InParams["fund_type"] = "mch";
	}
	else if(m_InParams["type"] == "2")
	{
		m_InParams["fund_type"] = "factor";
	}
	else if(m_InParams["type"] == "3")
	{
		m_InParams["fund_type"] = "serv";
	}


}

void CRouteSettleQuery::QueryLIquidation()
{
	CDEBUG_LOG("-----begin--------");
	int iRet;
	ostringstream sWhereSql;
	SqlResultSet outMap;
	SqlResultMapVector billInfoVector;
	JsonList m_jsonList;

	int iCnt      = (STOI(m_InParams["page"])-1)* STOI(m_InParams["limit"]);

	sWhereSql << "and Ffund_type = '"<<m_InParams["fund_type"]<<"' ";

    if(!m_InParams["input_time"].empty()&& ! m_InParams["end_time"].empty())
    {
    	sWhereSql << "and Fbill_date between '" << m_InParams["input_time"] << "' and '" <<  m_InParams["end_time"] << "' ";
    }
    if(!m_InParams["fund_id"].empty())
    {
    	sWhereSql << "and Fmch_id = '" << m_InParams["fund_id"] << "' ";
    }
    if(!m_InParams["fund_name"].empty())
    {
    	sWhereSql << "and Fmch_name = '" << m_InParams["fund_name"] << "' ";
    }
    if(!m_InParams["pay_channel"].empty())
    {
    	sWhereSql << "and Fpay_channel = '" << m_InParams["pay_channel"] << "' ";
    }
    if(!m_InParams["account_status"].empty())
    {
    	sWhereSql << "and Faccount_status = '" << m_InParams["account_status"] << "' ";
    }


    sqlss.str("");
    sqlss << "select count(*) as count from "
      	  <<ROUTE_BILL_DB<<"."<<BILL_DISTRIBUTION
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
    	  <<"Fmch_id,Fmch_name,Ftrade_count,Ftrade_amount,"
    	  <<"Frefund_count,Frefund_amount,Fmch_fee,Ftrade_net_amount,Fpend_settle,Fshare_profit, "
    	  <<"Faccount_type,Faccount_status from "
    	  <<ROUTE_BILL_DB<<"."<<BILL_DISTRIBUTION
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
			AddJsonMap(BillJsonMap,"mch_id",billInfoVector[i]["Fmch_id"]);
			AddJsonMap(BillJsonMap,"mch_name",billInfoVector[i]["Fmch_name"]);
			AddJsonMap(BillJsonMap,"trade_count",billInfoVector[i]["Ftrade_count"]);
			AddJsonMap(BillJsonMap,"trade_amount",billInfoVector[i]["Ftrade_amount"]);

			AddJsonMap(BillJsonMap,"refund_count",billInfoVector[i]["Frefund_count"]);
			AddJsonMap(BillJsonMap,"refund_amount",billInfoVector[i]["Frefund_amount"]);
			AddJsonMap(BillJsonMap,"mch_fee",billInfoVector[i]["Fmch_fee"]);   //交易手续费
			AddJsonMap(BillJsonMap,"trade_net_amount",billInfoVector[i]["Ftrade_net_amount"]);   //交易净额
			AddJsonMap(BillJsonMap,"pend_settle",billInfoVector[i]["Fpend_settle"]);  //待结算金额
			AddJsonMap(BillJsonMap,"share_profit",billInfoVector[i]["Fshare_profit"]);

			AddJsonMap(BillJsonMap,"account_type",billInfoVector[i]["Faccount_type"]);
			AddJsonMap(BillJsonMap,"account_status",billInfoVector[i]["Faccount_status"]);
			//AddJsonMap(BillJsonMap,"account_type2",billInfoVector[i]["Faccount_type2"]);
			//AddJsonMap(BillJsonMap,"account_status2",billInfoVector[i]["Faccount_status2"]);

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



