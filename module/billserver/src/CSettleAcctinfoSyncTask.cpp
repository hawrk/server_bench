/*
 * CSettleAcctinfoSyncTask.cpp
 *
 *  Created on: 2017-06-23
 *      Author: 
 */

#include <sys/time.h>
#include "CSettleAcctinfoSyncTask.h"
#include "tools.h"
#include "msglogapi.h"
#include "../Base/Comm/UserInfoClient.h"
#include "log/clog.h"
#include "json_util.h"
#include <stdlib.h>
#include <algorithm>
#include "CSpeedPosConfig.h"
#include "apayErrorNo.h"

extern CSpeedPosServer g_cOrderServer;
extern TMsgLog g_stMsgLog;

INT32 CSettleAcctinfoSyncTask::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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

void CSettleAcctinfoSyncTask::Deal()
{
	CDEBUG_LOG("Begin ...");

	//同步结算账户信息
	SyncSettleAcctinfo();

	CDEBUG_LOG("End.");
}

/*
 * 解析出请求结构
 * 成功返回0 失败返回-1
 */
void CSettleAcctinfoSyncTask::FillReq( NameValueMap& mapInput)
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

void CSettleAcctinfoSyncTask::CheckInput()
{
	CDEBUG_LOG("Begin ...");

	if(m_InParams["ver"].empty() || m_InParams["ver"] != VERSION)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("cmd", m_InParams["cmd"], 1, 10, true);
	Check::CheckStrParam("bm_id", m_InParams["bm_id"], 1, 20, true);
	Check::CheckStrParam("id", m_InParams["id"], 1, 32, true);

	CDEBUG_LOG("End.");
}

void CSettleAcctinfoSyncTask::SetRetParam()
{
	CDEBUG_LOG("Begin ...");

	m_RetMap["id"] = m_InParams["id"];

	for(NameValueMapIter iter = m_RetMap.begin(); iter != m_RetMap.end(); iter++)
	{
		m_ContentJsonMap.insert(JsonMap::value_type(JsonType(iter->first), JsonType(iter->second)));
	}

	CDEBUG_LOG("End.");
}

void CSettleAcctinfoSyncTask::BuildResp( CHAR** outbuf, INT32& outlen )
{
	CDEBUG_LOG("Begin ...");

    CHAR szResp[ MAX_RESP_LEN ];
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

void CSettleAcctinfoSyncTask::LogProcess()
{
}

void CSettleAcctinfoSyncTask::SyncSettleAcctinfo()
{
	CDEBUG_LOG("Begin ...");

	INT32 iRet = 0;
	ostringstream sqlss;
	ostringstream modiss;
	SqlResultSet billSettleInfo;
	modiss.str("");

	//校验记录是否存在
	sqlss.str("");
	sqlss << "select id,fund_type,partner_id,partner_name,settle_status from " 
		<< BILL_DB << "." << BILL_SETTLE << " where "
		<< "id = '" << CMySQL::EscapeStr(m_InParams["id"]) << "'"
		<< ";";
	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB, sqlss.str().c_str(), billSettleInfo);
	if ( 0 == iRet )
	{
		CERROR_LOG("bill_settle id[%s] not exist!", m_InParams["id"].c_str());
        throw(CTrsExp(ERR_BILL_SETTLE_NOT_EXIST, "结算单不存在!"));
	}

	//只有失败的结算单才能同步
	if( "4" != billSettleInfo["settle_status"] )
	{
		CERROR_LOG("settle_status[%s] error!", billSettleInfo["settle_status"].c_str());
        throw(CTrsExp(ERR_BILL_SETTLE_STATUS, "当前状态不允许同步!"));
	}
	m_RetMap["settle_status"] = billSettleInfo["settle_status"];

	//查询商户进件信息
	TRemitBill remitBill;
	remitBill.Reset();
	remitBill.account_id = billSettleInfo["partner_id"];
	remitBill.sType      = billSettleInfo["fund_type"];
	iRet = g_cOrderServer.CallGetBankNoApi(remitBill);
	if (iRet < 0)
	{
		CERROR_LOG("CallGetBankNoApi failed! iRet[%d].\n",iRet);
		throw(CTrsExp(ERR_NOTIFY_SETTLE_FAILED,"CallGetBankNoApi failed!"));
	}

	//修改项
	if(!remitBill.sBankType.empty())
	{
		modiss << ", bank_type = '" << CMySQL::EscapeStr(remitBill.sBankType) << "'";
	}
	if(!remitBill.sBankCardNo.empty())
	{
		modiss << ", bank_cardno = '" << CMySQL::EscapeStr(remitBill.sBankCardNo) << "'";
	}
	if(!remitBill.sName.empty())
	{
		modiss << ", card_name = '" << CMySQL::EscapeStr(remitBill.sName) << "'";
	}
	if(!remitBill.sBankCardType.empty())
	{
		modiss << ", card_type = " << CMySQL::EscapeStr(remitBill.sBankCardType);
	}
	//if(!m_InParams["bank_flag"].empty())
	//{
	//	modiss << ", bank_flag = " << CMySQL::EscapeStr(m_InParams["bank_flag"]);
	//}
	if(!remitBill.sBranchNo.empty())
	{
		modiss << ", bank_inscode = '" << CMySQL::EscapeStr(remitBill.sBranchNo) << "'";
	}

	if ( modiss.str().empty() )
	{
		CDEBUG_LOG("no option to be changed!");
		CDEBUG_LOG("End.");
		return;
	}

	//修改信息
	sqlss.str("");
	sqlss << "update " << BILL_DB << "." << BILL_SETTLE << " set"
		  << " id = '" << CMySQL::EscapeStr(m_InParams["id"]) << "'"
		  << modiss.str()
		  << ", modify_time = now()"
		  << " where id = '" << CMySQL::EscapeStr(m_InParams["id"]) << "'"
		  << ";";
	iRet = m_mysql.Execute(*m_pBillDB, sqlss.str().c_str());
	if ( 1 == iRet && 1 != m_mysql.getAffectedRows() )
	{
		CERROR_LOG("update db error! affected rows=[%d]", m_mysql.getAffectedRows());
        throw(CTrsExp(UPDATE_DB_ERR, "update db error!"));
	}
	CDEBUG_LOG("affectedrows[%d]", m_mysql.getAffectedRows());	

	CDEBUG_LOG("End.");
}

