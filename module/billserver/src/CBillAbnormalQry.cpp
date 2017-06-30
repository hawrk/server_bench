/*
 * CBillAbnormalQry.cpp
 *
 *  Created on: 2017年6月9日
 *      Author: hawrkchen
 */

#include "CBillAbnormalQry.h"


CBillAbnormalQry::CBillAbnormalQry()
{
	Reset();
}

INT32 CBillAbnormalQry::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
{
    BEGIN_LOG(__func__);
    CDEBUG_LOG("------------process begin----------");
    INT32 iRet = 0;

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
		iRet = FillReq(mapInput);
		if ( iRet != 0 )
		{
			m_iRetCode = iRet;
			BuildResp(outbuf, outlen);
			return m_iRetCode;
		}
		//校验请求
		iRet = CheckInput();
		if ( iRet != 0 )
		{
			snprintf( m_szErrMsg, sizeof(m_szErrMsg),
					  "CBillAbnormalQry::Execute CheckInput Failed.Ret[%d]", iRet );
			m_iRetCode = iRet;
			BuildResp(outbuf, outlen);
			return m_iRetCode;
		}

		DealQueryAbnormal();


    }
    catch(CTrsExp& e)
    {

		m_stResp.err_code = atoi(e.retcode.c_str());
		m_stResp.err_msg = e.retmsg;

		BuildResp(outbuf, outlen);
		CDEBUG_LOG("------------exception process end----------");
		return m_stResp.err_code;
    }
    catch(...)
    {
		m_stResp.err_code = -1;
		m_stResp.err_msg = "Unknown Exception";
		BuildResp(outbuf, outlen);
		CDEBUG_LOG("------------exception process end----------");
		return m_stResp.err_code;
    }


	m_stResp.err_code = 0;
	m_stResp.err_msg = RESP_SUCCUSS_MSG;
    BuildResp( outbuf, outlen );
    CDEBUG_LOG("------------ process end----------");

    return m_iRetCode;
}


/*
 * 解析出请求结构
 * 成功返回0 失败返回-1
 */
INT32 CBillAbnormalQry::FillReq( NameValueMap& mapInput)
{
	NameValueMapIter iter;
	for(iter = mapInput.begin();iter!=mapInput.end();iter++)
	{
		string szName = iter->first;
		transform(szName.begin(), szName.end(), szName.begin(), ::tolower);
		m_InParams[szName] = getSafeInput(iter->second);
	}

    return 0;
}

INT32 CBillAbnormalQry::CheckInput()
{
	if(m_InParams["ver"].empty() || m_InParams["ver"] != VERSION)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}
	Check::CheckStrParam("bm_id", m_InParams["bm_id"], 1, 10,true);

	Check::CheckStrParam("input_time", m_InParams["input_time"], 1, 10);
	Check::CheckStrParam("end_time", m_InParams["end_time"], 1, 10);
	Check::CheckStrParam("pay_channel", m_InParams["pay_channel"], 1, 10);

	Check::CheckDigitalParam("bill_status", m_InParams["bill_status"], 1, 5);
	Check::CheckDigitalParam("proc_status", m_InParams["proc_status"], 1, 5);

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
    return 0;
}

void CBillAbnormalQry::DealQueryAbnormal()
{
	CDEBUG_LOG("-----begin--------");
	int iRet;
	clib_mysql* pBillDb = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
	CMySQL m_mysql;
	ostringstream sqlss;
	ostringstream sWhereSql;
	SqlResultSet outMap;
	SqlResultMapVector billInfoVector;
	JsonList m_jsonList;

	int iCnt      = (STOI(m_InParams["page"])-1)* STOI(m_InParams["limit"]);

    if(!m_InParams["input_time"].empty()&& ! m_InParams["end_time"].empty())
    {
    	sWhereSql << "and bill_date between '" << m_InParams["input_time"] << "' and '" <<  m_InParams["end_time"] << "' ";
    }
    if(!m_InParams["pay_channel"].empty())
    {
    	sWhereSql << "and pay_channel = '" << m_InParams["pay_channel"] << "' ";
    }
    if(!m_InParams["bill_status"].empty())
    {
    	sWhereSql << "and order_status = '"<< m_InParams["bill_status"]<<"' ";
    }
    if(!m_InParams["proc_status"].empty())
    {
    	sWhereSql << "and porcess_status = '"<< m_InParams["proc_status"]<<"' ";
    }


    sqlss.str("");
    sqlss << "select count(*) as count from "
      	  <<BILL_DB<<"."<<BILL_ABNORMAL
		  <<" where bm_id = '"<<m_InParams["bm_id"]<<"' "
  		  << sWhereSql.str()
		  <<";";
    iRet = m_mysql.QryAndFetchResMap(*pBillDb,sqlss.str().c_str(),outMap);

    if(outMap["count"] == "0")  //没有记录，直接返回
    {
        m_ContentJsonMap.clear();
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outMap["count"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("page"), JsonType(m_InParams["page"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType("")));
    	return ;
    }

    sqlss.str("");
    sqlss <<"select bill_date,bill_batch_no,pay_channel,bank_inscode,mch_id,"
    		"pf_trade_amount,pf_refund_amount,pf_order_no,pf_refund_no,"
    		"ch_trade_amount,ch_refund_amount,ch_order_no,ch_refund_no,"
    		"trade_date,order_status,abnormal_type,porcess_status,create_time,modify_time from "
    	  <<BILL_DB<<"."<<BILL_ABNORMAL
		  <<" where bm_id = '"<<m_InParams["bm_id"]<<"' "
		  << sWhereSql.str()
		  << " limit " << iCnt << "," << m_InParams["limit"] << ";";

    iRet = m_mysql.QryAndFetchResMVector(*pBillDb,sqlss.str().c_str(),billInfoVector);
    if(iRet == 1)  //有记录
    {
    	JsonMap BillJsonMap;
    	m_jsonList.clear();
		for(unsigned int i = 0; i < billInfoVector.size(); i++)
		{
			BillJsonMap.clear();
			AddJsonMap(BillJsonMap,"bill_date",billInfoVector[i]["bill_date"]);
			AddJsonMap(BillJsonMap,"bill_batch_no",billInfoVector[i]["bill_batch_no"]);
			AddJsonMap(BillJsonMap,"pay_channel",billInfoVector[i]["pay_channel"]);
			AddJsonMap(BillJsonMap,"bank_inscode",billInfoVector[i]["bank_inscode"]);
			AddJsonMap(BillJsonMap,"mch_id",billInfoVector[i]["mch_id"]);
			AddJsonMap(BillJsonMap,"pf_trade_amount",billInfoVector[i]["pf_trade_amount"]);
			AddJsonMap(BillJsonMap,"pf_refund_amount",billInfoVector[i]["pf_refund_amount"]);
			AddJsonMap(BillJsonMap,"pf_order_no",billInfoVector[i]["pf_order_no"]);
			AddJsonMap(BillJsonMap,"pf_refund_no",billInfoVector[i]["pf_refund_no"]);

			AddJsonMap(BillJsonMap,"ch_trade_amount",billInfoVector[i]["ch_trade_amount"]);
			AddJsonMap(BillJsonMap,"ch_refund_amount",billInfoVector[i]["ch_refund_amount"]);
			AddJsonMap(BillJsonMap,"ch_order_no",billInfoVector[i]["ch_order_no"]);
			AddJsonMap(BillJsonMap,"ch_refund_no",billInfoVector[i]["ch_refund_no"]);
			AddJsonMap(BillJsonMap,"trade_date",billInfoVector[i]["trade_date"]);
			AddJsonMap(BillJsonMap,"abnormal_type",billInfoVector[i]["abnormal_type"]);
			AddJsonMap(BillJsonMap,"order_status",billInfoVector[i]["order_status"]);
			AddJsonMap(BillJsonMap,"porcess_status",billInfoVector[i]["porcess_status"]);
			AddJsonMap(BillJsonMap,"create_time",billInfoVector[i]["create_time"]);
			AddJsonMap(BillJsonMap,"modify_time",billInfoVector[i]["modify_time"]);

			m_jsonList.push_front(JsonList::value_type(BillJsonMap));
		}

    }
    m_ContentJsonMap.clear();
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outMap["count"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("page"), JsonType(m_InParams["page"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType(m_jsonList)));

	CDEBUG_LOG("-----end--------");
}

void CBillAbnormalQry::BuildResp( CHAR** outbuf, INT32& outlen )
{
    CHAR szResp[ MAX_RESP_LEN ];
    //CHAR szResult[ MAX_RESP_LEN ];
	JsonMap jsonRsp;
	jsonRsp.insert(JsonMap::value_type(JsonType("retcode"), JsonType((double)m_stResp.err_code)));
	jsonRsp.insert(JsonMap::value_type(JsonType("retmsg"), JsonType(m_stResp.err_msg)));
	if (!m_ContentJsonMap.empty())
		jsonRsp.insert(JsonMap::value_type(JsonType("Content"), JsonType(m_ContentJsonMap)));
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

void CBillAbnormalQry::LogProcess()
{

}
