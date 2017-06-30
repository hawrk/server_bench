/*
 * CBillSummaryQry.cpp
 *
 *  Created on: 2017年6月9日
 *      Author: hawrkchen
 *      Desc : 对账总览
 */

#ifndef _CBILLSUMMARYQRY_CPP_
#define _CBILLSUMMARYQRY_CPP_

#include "CBillSummaryQry.h"

CBillSummaryQry::CBillSummaryQry()
{
	Reset();
	m_ContentJsonMap.clear();
}

INT32 CBillSummaryQry::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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
					  "CBillSummaryQry::Execute CheckInput Failed.Ret[%d]", iRet );
			m_iRetCode = iRet;
			BuildResp(outbuf, outlen);
			return m_iRetCode;
		}

		if(m_InParams["step"] == "1")   //STEP=1 页头的汇总数据
		{
			DealQuerySummaryCount();
		}
		if(m_InParams["step"] == "2"||m_InParams["step"] == "3")  //STEP=2列表的汇总数据
		{
			DealQuerySummary();
		}


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
INT32 CBillSummaryQry::FillReq( NameValueMap& mapInput)
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

INT32 CBillSummaryQry::CheckInput()
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
	Check::CheckStrParam("bill_batch_no", m_InParams["bill_batch_no"], 1, 10);
	Check::CheckDigitalParam("bill_result", m_InParams["bill_result"], 1, 5);

	Check::CheckStrParam("step", m_InParams["step"], 1, 3);

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

void CBillSummaryQry::DealQuerySummaryCount()
{
	CDEBUG_LOG("-----begin--------");
	int iRet;
	clib_mysql* pBillDb = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
	CMySQL m_mysql;
	ostringstream sqlss;
	ostringstream sWhereSql;
	SqlResultSet outMap;
	JsonList m_jsonList;

    if(!m_InParams["input_time"].empty()&& ! m_InParams["end_time"].empty())
    {
    	sWhereSql << "and bill_date between '" << m_InParams["input_time"] << "' and '" <<  m_InParams["end_time"] << "' ";
    }
    if(!m_InParams["pay_channel"].empty())
    {
    	sWhereSql << "and pay_channel = '" << m_InParams["pay_channel"] << "' ";
    }

    if(!m_InParams["bill_result"].empty())
    {
    	sWhereSql << "and bill_result = '"<< m_InParams["bill_result"]<<"' ";
    }
    if(!m_InParams["bill_batch_no"].empty())
    {
    	sWhereSql << "and bill_batch_no = '"<< m_InParams["bill_batch_no"]<<"' ";
    }

    sqlss.str("");
    sqlss <<"select sum(pf_total_amount-pf_total_refund_amount) as net_amount,"
    		"sum(pf_total_fee-pf_refund_fee) as net_fee,"
    		"sum(pf_total_amount-pf_total_refund_amount-pf_total_fee+pf_refund_fee) as net_pay_amount,"
    		"sum(pf_total_count) as total_count,"
    		"sum(pf_total_amount) as total_amount,"
    		"sum(pf_total_refund_count) as total_refund_count,"
    		"sum(pf_total_refund_amount) as total_refund_amount,"
    		"sum(abnormal_count) as abnormal_count,"
    		"sum(abnormal_amount) as abnormal_amount from "
    	  <<BILL_DB<<"."<<BILL_SUMMARY
		  <<" where bm_id = '"<<m_InParams["bm_id"]<<"' "
		  << sWhereSql.str()
		  <<";";

    iRet = m_mysql.QryAndFetchResMap(*pBillDb,sqlss.str().c_str(),outMap);
    if(iRet == 1)  //有记录
    {
    	//m_jsonList.clear();
    	//m_ContentJsonMap.clear();

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

void CBillSummaryQry::DealQuerySummary()
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
    if(!m_InParams["bill_result"].empty())
    {
    	sWhereSql << "and bill_result = '"<< m_InParams["bill_result"]<<"' ";
    }
    if(!m_InParams["bill_batch_no"].empty())
    {
    	sWhereSql << "and bill_batch_no = '"<< m_InParams["bill_batch_no"]<<"' ";
    }

    sqlss.str("");
    sqlss << "select count(*) as count from "
      	  <<BILL_DB<<"."<<BILL_SUMMARY
		  <<" where bm_id = '"<<m_InParams["bm_id"]<<"' "
  		  << sWhereSql.str()
		  <<";";
    iRet = m_mysql.QryAndFetchResMap(*pBillDb,sqlss.str().c_str(),outMap);

    if(outMap["count"] == "0")  //没有记录，直接返回
    {
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outMap["count"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("page"), JsonType(m_InParams["page"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType("")));
    	return ;
    }

    sqlss.str("");
    sqlss <<"select bill_date,bill_batch_no,pay_channel,bank_inscode,bank_settle_acct,"
    		"pf_total_count,pf_total_amount,pf_total_refund_count,pf_total_refund_amount,pf_total_fee,pf_refund_fee,"
    		"ch_total_count,ch_total_amount,ch_total_refund_count,ch_total_refund_amount,ch_total_fee,ch_refund_fee,"
    		"abnormal_count,abnormal_amount,normal_total_amount,normal_refund_amount,bill_result,create_time from "
    	  <<BILL_DB<<"."<<BILL_SUMMARY
		  <<" where bm_id = '"<<m_InParams["bm_id"]<<"' "
		  << sWhereSql.str()
		  << " limit " << iCnt << "," << m_InParams["limit"]  << ";";

    iRet = m_mysql.QryAndFetchResMVector(*pBillDb,sqlss.str().c_str(),billInfoVector);
    if(iRet == 1)  //有记录
    {
    	JsonMap BillJsonMap;
    	m_jsonList.clear();
		for(unsigned int i = 0; i < billInfoVector.size(); i++)
		{
			BillJsonMap.clear();
			BillJsonMap.insert(JsonMap::value_type(JsonType("bill_date"), JsonType(billInfoVector[i]["bill_date"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("bill_batch_no"), JsonType(billInfoVector[i]["bill_batch_no"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pay_channel"), JsonType(billInfoVector[i]["pay_channel"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("bank_inscode"), JsonType(billInfoVector[i]["bank_inscode"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("bank_settle_acct"), JsonType(billInfoVector[i]["bank_settle_acct"])));

			BillJsonMap.insert(JsonMap::value_type(JsonType("pf_total_count"), JsonType(billInfoVector[i]["pf_total_count"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pf_total_amount"), JsonType(billInfoVector[i]["pf_total_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pf_total_refund_count"), JsonType(billInfoVector[i]["pf_total_refund_count"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pf_total_refund_amount"), JsonType(billInfoVector[i]["pf_total_refund_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pf_total_fee"), JsonType(billInfoVector[i]["pf_total_fee"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("pf_refund_fee"), JsonType(billInfoVector[i]["pf_refund_fee"])));

			BillJsonMap.insert(JsonMap::value_type(JsonType("ch_total_count"), JsonType(billInfoVector[i]["ch_total_count"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("ch_total_amount"), JsonType(billInfoVector[i]["ch_total_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("ch_total_refund_count"), JsonType(billInfoVector[i]["ch_total_refund_count"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("ch_total_refund_amount"), JsonType(billInfoVector[i]["ch_total_refund_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("ch_total_fee"), JsonType(billInfoVector[i]["ch_total_fee"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("ch_refund_fee"), JsonType(billInfoVector[i]["ch_refund_fee"])));

			BillJsonMap.insert(JsonMap::value_type(JsonType("abnormal_count"), JsonType(billInfoVector[i]["abnormal_count"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("abnormal_amount"), JsonType(billInfoVector[i]["abnormal_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("normal_total_amount"), JsonType(billInfoVector[i]["normal_total_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("normal_refund_amount"), JsonType(billInfoVector[i]["normal_refund_amount"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("bill_result"), JsonType(billInfoVector[i]["bill_result"])));
			m_jsonList.push_front(JsonList::value_type(BillJsonMap));
		}

    }
    m_ContentJsonMap.clear();
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"),JsonType(outMap["count"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("page"), JsonType(m_InParams["page"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType(m_jsonList)));

	CDEBUG_LOG("-----end--------");
}

void CBillSummaryQry::BuildResp( CHAR** outbuf, INT32& outlen )
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

void CBillSummaryQry::LogProcess()
{

}



#endif /* _CBILLSUMMARYQRY_CPP_ */
