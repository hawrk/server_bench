/*
 * CSettleQryTask.cpp
 *
 *  Created on: 2017-06-14
 *      Author: 
 */

#include <sys/time.h>
#include "CSettleQryTask.h"
#include "tools.h"
#include "msglogapi.h"
#include "../Base/Comm/UserInfoClient.h"
#include "log/clog.h"
#include "json_util.h"
#include <stdlib.h>
#include <algorithm>
#include "CSpeedPosConfig.h"
#include "apayErrorNo.h"


extern TMsgLog g_stMsgLog;

INT32 CSettleQryTask::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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

		//业务处理
		Deal();

		//设置返回参数
		SetRetParam();
	}
	catch(CTrsExp e)
	{
		m_RetMap.clear();
		m_RetMap["err_code"] = e.retcode;
		m_RetMap["err_msg"]  = e.retmsg;
	}
	
    BuildResp( outbuf, outlen );

	CDEBUG_LOG("End.");
    return atoi(m_RetMap["err_code"].c_str());
}

void CSettleQryTask::Deal()
{
	CDEBUG_LOG("Begin ...");

	if ( "1" == m_InParams["oper_type"] ) // 1-查询商户交易结算列表信息
	{
		QryMchSettleList();
	}
	else if ( "2" == m_InParams["oper_type"] ) // 2-查询结算差错列表信息
	{
		QrySettleErrorList();
	}
	else if ( "3" == m_InParams["oper_type"] ) // 3-查询结算差错挂账列表信息
	{
		QrySettleOnAccountList();
	}
	else if ( "4" == m_InParams["oper_type"] ) // 4-查询机构分润结算列表信息
	{
		QryInstiSettleList();
	}

	CDEBUG_LOG("End.");
}

/*
 * 解析出请求结构
 * 成功返回0 失败返回-1
 */
void CSettleQryTask::FillReq( NameValueMap& mapInput)
{
	CDEBUG_LOG("Begin ...");
	
	NameValueMapIter iter;
	for(iter=mapInput.begin();iter!=mapInput.end();iter++)
	{
		string szName = iter->first;
		transform(szName.begin(), szName.end(), szName.begin(), ::tolower); 
		m_InParams[szName] = getSafeInput(iter->second);
	}

	CDEBUG_LOG("End.");
}

void CSettleQryTask::CheckInput()
{
	CDEBUG_LOG("Begin ...");

	if(m_InParams["ver"].empty() || m_InParams["ver"] != "1.0")
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("cmd", m_InParams["cmd"], 1, 10, true);
	Check::CheckDigitalParam("oper_type", m_InParams["oper_type"], 1, 10, true);
	Check::CheckPage(m_InParams["page"]);
	Check::CheckLimit(m_InParams["limit"]);

	if ( "1" == m_InParams["oper_type"] )
	{
		Check::CheckStrParam("partner_id", m_InParams["partner_id"], 1, 32);
		Check::CheckStrParam("partner_name", m_InParams["partner_name"], 1, 128);
		Check::CheckDigitalParam("settle_status", m_InParams["settle_status"], 1, 10);
		Check::CheckDigitalParam("card_type", m_InParams["card_type"], 0, 1);
		Check::CheckDigitalParam("bank_flag", m_InParams["bank_flag"], 1, 2);
		Check::CheckStrParam("bill_date_start", m_InParams["bill_date_start"], 1, 10);
		Check::CheckStrParam("bill_date_end", m_InParams["bill_date_end"], 1, 10);
	}
	else if ( "2" == m_InParams["oper_type"] )
	{
		Check::CheckStrParam("partner_id", m_InParams["partner_id"], 1, 32);
		Check::CheckStrParam("partner_name", m_InParams["partner_name"], 1, 128);
		Check::CheckStrParam("pay_channel", m_InParams["pay_channel"], 1, 10);
		Check::CheckDigitalParam("card_type", m_InParams["card_type"], 0, 1);
		Check::CheckStrParam("bill_date_start", m_InParams["bill_date_start"], 1, 10);
		Check::CheckStrParam("bill_date_end", m_InParams["bill_date_end"], 1, 10);
	}
	else if ( "3" == m_InParams["oper_type"] )
	{
		Check::CheckStrParam("partner_id", m_InParams["partner_id"], 1, 32);
		Check::CheckStrParam("partner_name", m_InParams["partner_name"], 1, 128);
		Check::CheckStrParam("pay_channel", m_InParams["pay_channel"], 1, 10);
		Check::CheckDigitalParam("card_type", m_InParams["card_type"], 0, 1);
		Check::CheckStrParam("bill_date_start", m_InParams["bill_date_start"], 1, 10);
		Check::CheckStrParam("bill_date_end", m_InParams["bill_date_end"], 1, 10);
	}
	else if ( "4" == m_InParams["oper_type"] )
	{
		Check::CheckStrParam("partner_id", m_InParams["partner_id"], 1, 32);
		Check::CheckStrParam("partner_name", m_InParams["partner_name"], 1, 128);
		Check::CheckDigitalParam("settle_status", m_InParams["settle_status"], 1, 10);
		Check::CheckDigitalParam("card_type", m_InParams["card_type"], 0, 1);
		Check::CheckDigitalParam("bank_flag", m_InParams["bank_flag"], 1, 2);
		Check::CheckStrParam("bill_date_start", m_InParams["bill_date_start"], 1, 10);
		Check::CheckStrParam("bill_date_end", m_InParams["bill_date_end"], 1, 10);
	}	
	
	if ( m_InParams["page"].empty() )
	{
		m_InParams["page"] = "1";
	}
	if ( m_InParams["limit"].empty() )
	{
		m_InParams["limit"] = ITOS(SPDB_LIMIT_DEFAULT);
	}
	CDEBUG_LOG("page=[%s],limit=[%s]", m_InParams["page"].c_str(), m_InParams["limit"].c_str());

	CDEBUG_LOG("End.");
}

void CSettleQryTask::SetRetParam()
{
	CDEBUG_LOG("Begin ...");

	for(NameValueMapIter iter = m_RetMap.begin(); iter != m_RetMap.end(); iter++)
	{
		m_ContentJsonMap.insert(JsonMap::value_type(JsonType(iter->first), JsonType(iter->second)));
	}

	CDEBUG_LOG("End.");
}

void CSettleQryTask::BuildResp( CHAR** outbuf, INT32& outlen )
{
	CDEBUG_LOG("Begin ...");

    CHAR szResp[ _MAX_RESP_LEN ];
	JsonMap jsonRsp;

	if(m_RetMap["err_code"].empty())
	{
		m_RetMap["err_code"] = "00";
		m_RetMap["err_msg"]  = "success";
	}
	else    
    {
    }

	jsonRsp.insert(JsonMap::value_type(JsonType("err_code"), JsonType(m_RetMap["err_code"])));
	jsonRsp.insert(JsonMap::value_type(JsonType("err_msg"), JsonType(m_RetMap["err_msg"])));
	jsonRsp.insert(JsonMap::value_type(JsonType("content"), JsonType(m_ContentJsonMap)));
	
	std::string resContent = JsonUtil::objectToString(jsonRsp);

    gettimeofday(&m_stEnd, NULL);
    INT32 iUsedTime = CalDiff(m_stStart, m_stEnd);
	CDEBUG_LOG("use millisecond = [%d]", iUsedTime);
	snprintf(szResp, sizeof(szResp), //remaincount=1
		"%s\r\n",
		resContent.c_str());
	CDEBUG_LOG("ret msg=[%s]", szResp);

	outlen = strlen(szResp);
	*outbuf = (char*)malloc(outlen);
	memcpy(*outbuf, szResp, outlen);

	CDEBUG_LOG("End.");
}

void CSettleQryTask::LogProcess()
{
}

void CSettleQryTask::QryMchSettleList()
{
	CDEBUG_LOG("Begin ...");

	INT32 iRet = 0;
	ostringstream sqlss;
	ostringstream ssConds;
	SqlResultSet billSettleInfo;
	SqlResultMapVector billSettleInfoVector;
	string outLimit, outTotal, total_settle_amt;
	
	int iPageSize = STOI(m_InParams["limit"].c_str());
	int iPageNum  = STOI(m_InParams["page"].c_str());
    int iCnt      = (iPageNum-1)*iPageSize;

	if ( !m_InParams["partner_id"].empty() )
	{
		ssConds << " and partner_id = '" << CMySQL::EscapeStr(m_InParams["partner_id"]) << "'";
	}
	if ( !m_InParams["partner_name"].empty() )
	{
		ssConds << " and partner_name like '%" << CMySQL::EscapeStr(m_InParams["partner_name"]) << "%'";
	}
	if ( !m_InParams["settle_status"].empty() )
	{
		ssConds << " and settle_status = " << CMySQL::EscapeStr(m_InParams["settle_status"]);
	}
	if ( !m_InParams["card_type"].empty() )
	{
		ssConds << " and card_type = " << CMySQL::EscapeStr(m_InParams["card_type"]);
	}
	if ( !m_InParams["bank_flag"].empty() )
	{
		ssConds << " and bank_flag = " << CMySQL::EscapeStr(m_InParams["bank_flag"]);
	}
	if ( !m_InParams["bill_date_start"].empty() )
	{
		ssConds << " and bill_date >= '" << CMySQL::EscapeStr(m_InParams["bill_date_start"]) << "'";
	}
	if ( !m_InParams["bill_date_end"].empty() )
	{
		ssConds << " and bill_date <= '" << CMySQL::EscapeStr(m_InParams["bill_date_end"]) << "'";
	}
	
	//获取总记录数
	sqlss.str("");
	sqlss << "select count(id) as total_cnt, sum(settle_amt) as total_settle_amt from " 
		<< BILL_DB << "." << BILL_SETTLE
		<< " where 1=1"
		<< ssConds.str();
	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB, sqlss.str().c_str(), billSettleInfo);
	outTotal = billSettleInfo["total_cnt"];
	total_settle_amt = billSettleInfo["total_settle_amt"];

	//查询列表记录
	sqlss.str("");
	sqlss << "select \
			id,\
			bill_date,\
			trade_date,\
			bm_id,\
			batch_no,\
			card_type,\
			card_name,\
			bank_cardno,\
			bank_type,\
			bank_flag,\
			bank_inscode,\
			fund_type,\
			partner_id,\
			partner_name,\
			cycle,\
			pay_channel,\
			settle_amt,\
			settle_status,\
			settle_batchno,\
			detail_id,\
			settle_time,\
			create_time,\
			modify_time"
		  << " from " << BILL_DB << "." << BILL_SETTLE
		  << " where 1=1"
		  << ssConds.str()
		  << " order by create_time desc"
		  << " limit " << iCnt << "," << iPageSize << ";";
	iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB, sqlss.str().c_str(), billSettleInfoVector);

	if ( 1 == iRet ) //有记录
	{
		for(unsigned int i = 0; i < billSettleInfoVector.size(); i++)
		{
			JsonMap jsonTemp;
			for( SqlResultSet::iterator it = billSettleInfoVector[i].begin();
				 it != billSettleInfoVector[i].end();
				 it++
			)
			{
				jsonTemp.insert(JsonMap::value_type(JsonType(it->first), JsonType(it->second)));
			}
			m_retJsonLists.push_back(JsonList::value_type(jsonTemp));
		}
		outLimit = ITOS(billSettleInfoVector.size());
		CDEBUG_LOG("content=[%s]", JsonUtil::objectToString(m_retJsonLists).c_str());
	}
	else
	{
		outLimit = "0";
	}
	
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("page"), JsonType(m_InParams["page"])));
    m_ContentJsonMap.insert(JsonMap::value_type(JsonType("limit"), JsonType(outLimit)));
    m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outTotal)));
    m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType(m_retJsonLists)));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total_cnt"), JsonType(outTotal)));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total_settle_amt"), JsonType(total_settle_amt)));

	CDEBUG_LOG("End.");
}

void CSettleQryTask::QrySettleErrorList()
{
	CDEBUG_LOG("Begin ...");

	INT32 iRet = 0;
	ostringstream sqlss;
	ostringstream ssConds;
	SqlResultSet billSettleInfo;
	SqlResultMapVector billSettleInfoVector;
	string outLimit, outTotal, total_settle_amt;
	
	int iPageSize = STOI(m_InParams["limit"].c_str());
	int iPageNum  = STOI(m_InParams["page"].c_str());
    int iCnt      = (iPageNum-1)*iPageSize;
	
	if ( !m_InParams["partner_id"].empty() )
	{
		ssConds << " and partner_id = '" << CMySQL::EscapeStr(m_InParams["partner_id"]) << "'";
	}
	if ( !m_InParams["partner_name"].empty() )
	{
		ssConds << " and partner_name like '%" << CMySQL::EscapeStr(m_InParams["partner_name"]) << "%'";
	}
	if ( !m_InParams["pay_channel"].empty() )
	{
		ssConds << " and pay_channel = '" << CMySQL::EscapeStr(m_InParams["pay_channel"]) << "'";
	}
	if ( !m_InParams["card_type"].empty() )
	{
		ssConds << " and card_type = " << CMySQL::EscapeStr(m_InParams["card_type"]);
	}
	if ( !m_InParams["bill_date_start"].empty() )
	{
		ssConds << " and bill_date >= '" << CMySQL::EscapeStr(m_InParams["bill_date_start"]) << "'";
	}
	if ( !m_InParams["bill_date_end"].empty() )
	{
		ssConds << " and bill_date <= '" << CMySQL::EscapeStr(m_InParams["bill_date_end"]) << "'";
	}
	ssConds << " and error_type = 1";
	
	//获取总记录数
	sqlss.str("");
	sqlss << "select count(id) as total_cnt, sum(settle_amt) as total_settle_amt from " 
		<< BILL_DB << "." << BILL_SETTLE
		<< " where 1=1"
		<< ssConds.str();
	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB, sqlss.str().c_str(), billSettleInfo);
	outTotal = billSettleInfo["total_cnt"];
	total_settle_amt = billSettleInfo["total_settle_amt"];

	//查询列表记录
	sqlss.str("");
	sqlss << "select \
			id,\
			bill_date,\
			trade_date,\
			bm_id,\
			batch_no,\
			card_type,\
			card_name,\
			bank_cardno,\
			bank_type,\
			bank_flag,\
			bank_inscode,\
			fund_type,\
			partner_id,\
			partner_name,\
			cycle,\
			pay_channel,\
			settle_amt,\
			settle_status,\
			settle_batchno,\
			detail_id,\
			settle_time,\
			error_type,\
			err_create_time,\
			err_msg,\
			err_settle_time,\
			err_settle_batchno,\
			create_time,\
			modify_time"
		  << " from " << BILL_DB << "." << BILL_SETTLE
		  << " where 1=1"
		  << ssConds.str()
		  << " order by create_time desc"
		  << " limit " << iCnt << "," << iPageSize << ";";
	iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB, sqlss.str().c_str(), billSettleInfoVector);

	if ( 1 == iRet ) //有记录
	{
		for(unsigned int i = 0; i < billSettleInfoVector.size(); i++)
		{
			JsonMap jsonTemp;
			for( SqlResultSet::iterator it = billSettleInfoVector[i].begin();
				 it != billSettleInfoVector[i].end();
				 it++
			)
			{
				jsonTemp.insert(JsonMap::value_type(JsonType(it->first), JsonType(it->second)));
			}
			m_retJsonLists.push_back(JsonList::value_type(jsonTemp));
		}
		outLimit = ITOS(billSettleInfoVector.size());
		CDEBUG_LOG("content=[%s]", JsonUtil::objectToString(m_retJsonLists).c_str());
	}
	else
	{
		outLimit = "0";
	}
	
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("page"), JsonType(m_InParams["page"])));
    m_ContentJsonMap.insert(JsonMap::value_type(JsonType("limit"), JsonType(outLimit)));
    m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outTotal)));
    m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType(m_retJsonLists)));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total_cnt"), JsonType(outTotal)));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total_settle_amt"), JsonType(total_settle_amt)));

	CDEBUG_LOG("End.");
}

void CSettleQryTask::QrySettleOnAccountList()
{
	CDEBUG_LOG("Begin ...");

	INT32 iRet = 0;
	ostringstream sqlss;
	ostringstream ssConds;
	SqlResultSet billSettleInfo;
	SqlResultMapVector billSettleInfoVector;
	string outLimit, outTotal, total_settle_amt;
	
	int iPageSize = STOI(m_InParams["limit"].c_str());
	int iPageNum  = STOI(m_InParams["page"].c_str());
    int iCnt      = (iPageNum-1)*iPageSize;
	
	if ( !m_InParams["partner_id"].empty() )
	{
		ssConds << " and partner_id = '" << CMySQL::EscapeStr(m_InParams["partner_id"]) << "'";
	}
	if ( !m_InParams["partner_name"].empty() )
	{
		ssConds << " and partner_name like '%" << CMySQL::EscapeStr(m_InParams["partner_name"]) << "%'";
	}
	if ( !m_InParams["pay_channel"].empty() )
	{
		ssConds << " and pay_channel = '" << CMySQL::EscapeStr(m_InParams["pay_channel"]) << "'";
	}
	if ( !m_InParams["card_type"].empty() )
	{
		ssConds << " and card_type = " << CMySQL::EscapeStr(m_InParams["card_type"]);
	}
	if ( !m_InParams["bill_date_start"].empty() )
	{
		ssConds << " and bill_date >= '" << CMySQL::EscapeStr(m_InParams["bill_date_start"]) << "'";
	}
	if ( !m_InParams["bill_date_end"].empty() )
	{
		ssConds << " and bill_date <= '" << CMySQL::EscapeStr(m_InParams["bill_date_end"]) << "'";
	}
	ssConds << " and error_type = 2";
	
	//获取总记录数
	sqlss.str("");
	sqlss << "select count(id) as total_cnt, sum(settle_amt) as total_settle_amt from " 
		<< BILL_DB << "." << BILL_SETTLE
		<< " where 1=1"
		<< ssConds.str();
	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB, sqlss.str().c_str(), billSettleInfo);
	outTotal = billSettleInfo["total_cnt"];
	total_settle_amt = billSettleInfo["total_settle_amt"];

	//查询列表记录
	sqlss.str("");
	sqlss << "select \
			id,\
			bill_date,\
			trade_date,\
			bm_id,\
			batch_no,\
			card_type,\
			card_name,\
			bank_cardno,\
			bank_type,\
			bank_flag,\
			bank_inscode,\
			fund_type,\
			partner_id,\
			partner_name,\
			cycle,\
			pay_channel,\
			settle_amt,\
			settle_status,\
			settle_batchno,\
			detail_id,\
			settle_time,\
			error_type,\
			err_create_time,\
			err_msg,\
			err_settle_time,\
			err_settle_batchno,\
			create_time,\
			modify_time"
		  << " from " << BILL_DB << "." << BILL_SETTLE
		  << " where 1=1"
		  << ssConds.str()
		  << " order by create_time desc"
		  << " limit " << iCnt << "," << iPageSize << ";";
	iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB, sqlss.str().c_str(), billSettleInfoVector);

	if ( 1 == iRet ) //有记录
	{
		for(unsigned int i = 0; i < billSettleInfoVector.size(); i++)
		{
			JsonMap jsonTemp;
			for( SqlResultSet::iterator it = billSettleInfoVector[i].begin();
				 it != billSettleInfoVector[i].end();
				 it++
			)
			{
				jsonTemp.insert(JsonMap::value_type(JsonType(it->first), JsonType(it->second)));
			}
			m_retJsonLists.push_back(JsonList::value_type(jsonTemp));
		}
		outLimit = ITOS(billSettleInfoVector.size());
		CDEBUG_LOG("content=[%s]", JsonUtil::objectToString(m_retJsonLists).c_str());
	}
	else
	{
		outLimit = "0";
	}
	
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("page"), JsonType(m_InParams["page"])));
    m_ContentJsonMap.insert(JsonMap::value_type(JsonType("limit"), JsonType(outLimit)));
    m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outTotal)));
    m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType(m_retJsonLists)));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total_cnt"), JsonType(outTotal)));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total_settle_amt"), JsonType(total_settle_amt)));

	CDEBUG_LOG("End.");
}

void CSettleQryTask::QryInstiSettleList()
{
	CDEBUG_LOG("Begin ...");

	INT32 iRet = 0;
	ostringstream sqlss;
	ostringstream ssConds;
	SqlResultSet billSettleInfo;
	SqlResultMapVector billSettleInfoVector;
	string outLimit, outTotal, total_settle_amt;
	
	int iPageSize = STOI(m_InParams["limit"].c_str());
	int iPageNum  = STOI(m_InParams["page"].c_str());
    int iCnt      = (iPageNum-1)*iPageSize;

	if ( !m_InParams["partner_id"].empty() )
	{
		ssConds << " and partner_id = '" << CMySQL::EscapeStr(m_InParams["partner_id"]) << "'";
	}
	if ( !m_InParams["partner_name"].empty() )
	{
		ssConds << " and partner_name like '%" << CMySQL::EscapeStr(m_InParams["partner_name"]) << "%'";
	}
	if ( !m_InParams["settle_status"].empty() )
	{
		ssConds << " and settle_status = " << CMySQL::EscapeStr(m_InParams["settle_status"]);
	}
	if ( !m_InParams["card_type"].empty() )
	{
		ssConds << " and card_type = " << CMySQL::EscapeStr(m_InParams["card_type"]);
	}
	if ( !m_InParams["bank_flag"].empty() )
	{
		ssConds << " and bank_flag = " << CMySQL::EscapeStr(m_InParams["bank_flag"]);
	}
	if ( !m_InParams["bill_date_start"].empty() )
	{
		ssConds << " and bill_date >= '" << CMySQL::EscapeStr(m_InParams["bill_date_start"]) << "'";
	}
	if ( !m_InParams["bill_date_end"].empty() )
	{
		ssConds << " and bill_date <= '" << CMySQL::EscapeStr(m_InParams["bill_date_end"]) << "'";
	}
	
	//获取总记录数
	sqlss.str("");
	sqlss << "select count(id) as total_cnt, sum(settle_amt) as total_settle_amt from " 
		<< BILL_DB << "." << BILL_SETTLE
		<< " where 1=1"
		<< ssConds.str();
	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB, sqlss.str().c_str(), billSettleInfo);
	outTotal = billSettleInfo["total_cnt"];
	total_settle_amt = billSettleInfo["total_settle_amt"];

	//查询列表记录
	sqlss.str("");
	sqlss << "select \
			id,\
			bill_date,\
			trade_date,\
			bm_id,\
			batch_no,\
			card_type,\
			card_name,\
			bank_cardno,\
			bank_type,\
			bank_flag,\
			bank_inscode,\
			fund_type,\
			partner_id,\
			partner_name,\
			cycle,\
			pay_channel,\
			settle_amt,\
			settle_status,\
			settle_batchno,\
			detail_id,\
			settle_time,\
			create_time,\
			modify_time"
		  << " from " << BILL_DB << "." << BILL_SETTLE
		  << " where 1=1"
		  << ssConds.str()
		  << " order by create_time desc"
		  << " limit " << iCnt << "," << iPageSize << ";";
	iRet = m_mysql.QryAndFetchResMVector(*m_pBillDB, sqlss.str().c_str(), billSettleInfoVector);

	if ( 1 == iRet ) //有记录
	{
		for(unsigned int i = 0; i < billSettleInfoVector.size(); i++)
		{
			JsonMap jsonTemp;
			for( SqlResultSet::iterator it = billSettleInfoVector[i].begin();
				 it != billSettleInfoVector[i].end();
				 it++
			)
			{
				jsonTemp.insert(JsonMap::value_type(JsonType(it->first), JsonType(it->second)));
			}
			m_retJsonLists.push_back(JsonList::value_type(jsonTemp));
		}
		outLimit = ITOS(billSettleInfoVector.size());
		CDEBUG_LOG("content=[%s]", JsonUtil::objectToString(m_retJsonLists).c_str());
	}
	else
	{
		outLimit = "0";
	}
	
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("page"), JsonType(m_InParams["page"])));
    m_ContentJsonMap.insert(JsonMap::value_type(JsonType("limit"), JsonType(outLimit)));
    m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outTotal)));
    m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType(m_retJsonLists)));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total_cnt"), JsonType(outTotal)));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total_settle_amt"), JsonType(total_settle_amt)));

	CDEBUG_LOG("End.");

	CDEBUG_LOG("End.");
}

