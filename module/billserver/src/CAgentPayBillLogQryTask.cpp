/*
 * 
 CAgentPayBillLogQryTask.cpp
 *
 *  Created on: 2009-6-3
 *      Author: 
 */

#include <sys/time.h>
#include "CAgentPayBillLogQryTask.h"
#include "tools.h"
#include "msglogapi.h"
#include "../../Base/Comm/UserInfoClient.h"
#include "log/clog.h"
#include "../business/apayErrorNo.h"

extern TMsgLog g_stMsgLog;


INT32 CAPayBillLogQryTask::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
{
    BEGIN_LOG(__func__);

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

		SetRetParam();
	}
	catch(CTrsExp e)
	{
		m_RetMap.clear();
		m_RetMap["err_code"] = e.retcode;
		m_RetMap["err_msg"]  = e.retmsg;
	}

	BuildResp( outbuf, outlen );

    return atoi(m_RetMap["err_code"].c_str());
}


/*
 * 解析出请求结构
 * 成功返回0 失败返回-1
 */
void CAPayBillLogQryTask::FillReq( NameValueMap& mapInput)
{
    BEGIN_LOG(__func__);

	MapFirstToLower(mapInput,m_InParams);
}

void CAPayBillLogQryTask::CheckInput()
{
	BEGIN_LOG(__func__);
	
	if(m_InParams["ver"].empty() || m_InParams["ver"] != VER)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("cmd", m_InParams["cmd"], 1, 10, true);

	Check::CheckPage(m_InParams["page"]);
	if(m_InParams["page"].empty())
	{
		m_InParams["page"] = ITOS(PAGE_DEFAULT);
	}
	
	Check::CheckLimit(m_InParams["limit"]);
	if(m_InParams["limit"].empty())
	{
		m_InParams["limit"] = ITOS(LIMIT_DEFAULT);
	}

	Check::CheckIsDigitalParam("settle_date",m_InParams["settle_date"],8,8);

	Check::CheckIsDigitalParam("bill_date",m_InParams["bill_date"],8,8);

/*
	if(m_InParams["settle_date"].empty() && m_InParams["bill_date"].empty())
	{
		CERROR_LOG("settle_date and bill_date is both empty!");
		throw(CTrsExp(ERR_INVALID_PARAMS, "settle_date and bill_date is both empty!"));	
	}
*/
}

void CAPayBillLogQryTask::BuildResp( CHAR** outbuf, INT32& outlen )
{
	BEGIN_LOG(__func__);

    CHAR szResp[ MAX_RESP_LEN ];
	JsonMap jsonRsp; 

	if(m_RetMap["err_code"].empty())
	{
		m_RetMap["err_code"] = "00";
		m_RetMap["err_msg"]  = "";
	}
	else
	{		
		Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig()->ReadErrCodeFromFile(m_RetMap);
	}

	jsonRsp.insert(JsonMap::value_type(JsonType("err_code"), JsonType(m_RetMap["err_code"])));
	jsonRsp.insert(JsonMap::value_type(JsonType("err_msg"), JsonType(m_RetMap["err_msg"])));

	if(m_ContenJsonMap.size() > 0)
	{
		jsonRsp.insert(JsonMap::value_type(JsonType("content"), JsonType(m_ContenJsonMap)));
	}
	
	std::string resContent = JsonUtil::objectToString(jsonRsp);

	//resContent.erase(std::remove(resContent.begin(), resContent.end(), '\\'), resContent.end());
	CDEBUG_LOG("resContent=[%s]!\n",resContent.c_str());

    gettimeofday(&m_stEnd, NULL);
    INT32 iUsedTime = CalDiff(m_stStart, m_stEnd);
	CDEBUG_LOG("process use time=[%d]!\n",iUsedTime);
	snprintf(szResp, sizeof(szResp), //remaincount=1
		"%s\r\n",
		resContent.c_str());

	outlen = strlen(szResp);
	*outbuf = (char*)malloc(outlen);
	memcpy(*outbuf, szResp, outlen);
}

//查询总数
void CAPayBillLogQryTask::QryTotal()
{
	BEGIN_LOG(__func__);
	SqlResultSet cntMap;

	stringstream szSqlBuf("");

	m_DBConn = Singleton<CSpeedPosConfig>::GetInstance()->GetApayBillDB();

	szSqlBuf.str("");

	szSqlBuf	<<	" SELECT COUNT(1) AS total_cnt"
				<<	" FROM "
				<<	AGENT_PAY_BILL_DB << "." <<	APAY_BILL_LOG_TABLE
				<<	" WHERE 1=1 ";

	if(!m_InParams["settle_date"].empty())
	{	szSqlBuf	<<	" AND settle_date='"	<<	m_InParams["settle_date"]	<<	"'";	}
	
	if(!m_InParams["bill_date"].empty())
	{	szSqlBuf	<<	" AND bill_date='"	<<	m_InParams["bill_date"] <<	"'";	}
	
	m_SqlHandle.QryAndFetchResMap(*m_DBConn,szSqlBuf.str().c_str(),cntMap);

	m_RetMap["total"] = cntMap["total_cnt"];
}

//查列表
void CAPayBillLogQryTask::QryDeal()
{
	BEGIN_LOG(__func__);

	stringstream szSqlBuf("");

	int iPageSize = STOI(m_InParams["limit"].c_str());
	int iPageNum  = STOI(m_InParams["page"].c_str());
	int iCnt	  = (iPageNum-1)*iPageSize;

	szSqlBuf	<<	" SELECT settle_date,bill_date,pay_channel_id,pay_channel_name,total_fee,total_number,total_ref_fee,total_ref_number,bill_status,err_msg"
				<<	" FROM "
				<<	AGENT_PAY_BILL_DB << "." <<	APAY_BILL_LOG_TABLE
				<<	" WHERE 1=1 ";

	if(!m_InParams["settle_date"].empty())
	{	szSqlBuf	<<	" AND settle_date='"	<<	m_InParams["settle_date"]	<<	"'";	}
	
	if(!m_InParams["bill_date"].empty())
	{	szSqlBuf	<<	" AND bill_date='"	<<	m_InParams["bill_date"]	<<	"'";	}

	szSqlBuf	<<	" ORDER BY bill_date DESC";


	szSqlBuf	<<	" LIMIT " <<	iCnt	<<	"," <<	iPageSize	<<	";";

	m_DBConn = Singleton<CSpeedPosConfig>::GetInstance()->GetApayBillDB();
	m_SqlHandle.QryAndFetchResMVector(*m_DBConn,szSqlBuf.str().c_str(),m_resultMVet);
}

void CAPayBillLogQryTask::Deal()
{
	BEGIN_LOG(__func__);

	QryTotal();

	QryDeal();
}

void CAPayBillLogQryTask::SetRetParam()
{
	BEGIN_LOG(__func__);

	m_RetMap["page"]  = m_InParams["page"];
	m_RetMap["limit"] = m_InParams["limit"];
	//m_RetMap["total"] = ITOS(m_resultMVet.size());

	NameValueMapIter iter;
	for(iter=m_RetMap.begin();iter!=m_RetMap.end();iter++)
	{
		m_ContenJsonMap.insert(JsonMap::value_type(JsonType(iter->first), JsonType(iter->second)));
	}

	for(int i=m_resultMVet.size()-1;i>=0;i--)
	{
		JsonMap jsonTemp;
		for(NameValueMap::iterator mIter = m_resultMVet[i].begin();mIter != m_resultMVet[i].end();mIter++)
		{
			jsonTemp.insert(JsonMap::value_type(JsonType(mIter->first), JsonType(mIter->second)));
		}
		m_JsonList.push_front(JsonList::value_type(jsonTemp));
	}


/*
	vector<SqlResultSet>::iterator vIter = m_resultMVet.begin();

	for(;vIter != m_resultMVet.end();vIter++)
	{
		JsonMap jsonTemp;
		for(NameValueMap::iterator mIter = (*vIter).begin();mIter != (*vIter).end();mIter++)
		{
			jsonTemp.insert(JsonMap::value_type(JsonType(mIter->first), JsonType(mIter->second)));
		}
		m_JsonList.push_front(JsonList::value_type(jsonTemp));
	}
*/
	m_ContenJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType(m_JsonList)));
}


void CAPayBillLogQryTask::LogProcess()
{
   
}

