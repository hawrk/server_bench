/*
 * CMchBillQuery.cpp
 *
 *  Created on: 2017年7月21日
 *      Author: hawrkchen
 */

#include "CMchBillQuery.h"

CMchBillQuery::CMchBillQuery()
{
	//ctor
}
CMchBillQuery::~CMchBillQuery()
{
	//dector
}

INT32 CMchBillQuery::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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
			QueryMchBillSummary();
		}
		else if(m_InParams["step"] == "2")  //STEP=2列表的汇总数据
		{
			QueryMchBillRecord();
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
void CMchBillQuery::FillReq( NameValueMap& mapInput)
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

void CMchBillQuery::CheckInput()
{

	if(m_InParams["ver"].empty() || m_InParams["ver"] != VER)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("input_time", m_InParams["input_time"], 1, 10);
	Check::CheckStrParam("end_time", m_InParams["end_time"], 1, 10);
	Check::CheckStrParam("pay_channel", m_InParams["pay_channel"], 1, 10);
	Check::CheckStrParam("mch_id", m_InParams["mch_id"], 1, 10);

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

void CMchBillQuery::QueryMchBillSummary()
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
    if(!m_InParams["mch_id"].empty())
    {
    	sWhereSql << "and Fmch_id = '"<< m_InParams["mch_id"]<<"' ";
    }


    sqlss.str("");
    sqlss <<"select sum(Ftrade_count)  as trade_count,"
    		"sum(Ftrade_amount) as trade_amount,"
    		"sum(Frefund_count) as refund_count,"
    		"sum(Frefund_amount) as refund_amount,"
    		"sum(Fnet_amount) as net_amount,"
    		"sum(Ftotal_fee) as total_fee "
    		" from "
    	  <<ROUTE_BILL_DB<<"."<<MCH_CHECK_BILL
		  <<" where 1=1 "
		  << sWhereSql.str()
		  <<";";

    iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),outMap);
    if(iRet == 1)  //有记录
    {
    	m_ContentJsonMap.clear();

    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("trade_count"), JsonType(outMap["trade_count"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("trade_amount"), JsonType(outMap["trade_amount"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("refund_count"), JsonType(outMap["refund_count"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("refund_amount"), JsonType(outMap["refund_amount"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("net_amount"), JsonType(outMap["net_amount"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total_fee"), JsonType(outMap["total_fee"])));

    }

	CDEBUG_LOG("-----end--------");
}

void CMchBillQuery::QueryMchBillRecord()
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
    	sWhereSql << "and Fpay_channel_id = '" << m_InParams["pay_channel"] << "' ";
    }
    if(!m_InParams["mch_id"].empty())
    {
    	sWhereSql << "and Fmch_id = '"<< m_InParams["mch_id"]<<"' ";
    }

    sqlss.str("");
    sqlss << "select count(*) as count from "
      	  <<ROUTE_BILL_DB<<"."<<MCH_CHECK_BILL
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
    sqlss <<"select Fmch_id,Fbill_date,Fpay_channel_id,Fpay_channel,"
    		"Ftrade_count,Ftrade_amount,Frefund_count,Frefund_amount,"
    		"Fnet_amount,Ftotal_fee from "
    	  <<ROUTE_BILL_DB<<"."<<MCH_CHECK_BILL
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
			AddJsonMap(BillJsonMap,"mch_id",billInfoVector[i]["Fmch_id"]);
			AddJsonMap(BillJsonMap,"bill_date",billInfoVector[i]["Fbill_date"]);
			AddJsonMap(BillJsonMap,"pay_channel_id",billInfoVector[i]["Fpay_channel_id"]);
			AddJsonMap(BillJsonMap,"pay_channel",billInfoVector[i]["Fpay_channel"]);
			AddJsonMap(BillJsonMap,"trade_count",billInfoVector[i]["Ftrade_count"]);
			AddJsonMap(BillJsonMap,"trade_amount",billInfoVector[i]["Ftrade_amount"]);
			AddJsonMap(BillJsonMap,"refund_count",billInfoVector[i]["Frefund_count"]);
			AddJsonMap(BillJsonMap,"refund_amount",billInfoVector[i]["Frefund_amount"]);
			AddJsonMap(BillJsonMap,"net_amount",billInfoVector[i]["Fnet_amount"]);
			AddJsonMap(BillJsonMap,"total_fee",billInfoVector[i]["Ftotal_fee"]);

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


void CMchBillQuery::SetRetParam()
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

void CMchBillQuery::BuildResp( CHAR** outbuf, INT32& outlen )
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

void CMchBillQuery::LogProcess()
{
}

