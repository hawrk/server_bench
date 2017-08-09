/*
 * CDownLoadSettleFile.cpp
 *
 *  Created on: 2017年7月19日
 *      Author: hawrkchen
 */

#include "CDownLoadSettleFile.h"

INT32 CDownLoadSettleFile::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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

	    CallPayGate2DownSettleBill();

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
void CDownLoadSettleFile::FillReq( NameValueMap& mapInput)
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

void CDownLoadSettleFile::CheckInput()
{

	if(m_InParams["ver"].empty() || m_InParams["ver"] != VER)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("bill_date", m_InParams["bill_date"], 1, 10, true);  //结算日期
	Check::CheckStrParam("pay_channel", m_InParams["pay_channel"], 1, 10, true);  //结算通道


}

void CDownLoadSettleFile::CallPayGate2DownSettleBill()
{
	char szRecvBuff[1024] = {0};
	StringMap paramMap;
	StringMap recvMap;
	CSocket* paygateSocket = Singleton<CSpeedPosConfig>::GetInstance()->GetPaygateServerSocket();
	paramMap.insert(StringMap::value_type("order_date", m_InParams["bill_date"]));
	paramMap.insert(StringMap::value_type("pay_channel",m_InParams["pay_channel"]));

	paygateSocket->SendAndRecvLineEx(paramMap,szRecvBuff,sizeof(szRecvBuff),"\r\n");
	CDEBUG_LOG("recv Msg [%s]",szRecvBuff);

	if(NULL == szRecvBuff||strlen(szRecvBuff) == 0)  //
	{
		CDEBUG_LOG("send down load settle file fail !!,err_msg[%s]",recvMap["retmsg"].c_str());
		throw(CTrsExp(ERR_DOWNLOAD_SETTLE,"send down load settle file fail!!"));
	}

	Kv2Map(szRecvBuff,recvMap);
	CDEBUG_LOG("CallPayGate2DownCheckBill,retcode=[%s]",recvMap["retcode"].c_str());
	if(recvMap["retcode"] != "0")
	{
		CDEBUG_LOG("CallPayGate2DownSettleBill Fail!!,err_msg[%s]",recvMap["retmsg"].c_str());
		throw(CTrsExp(ERR_DOWNLOAD_SETTLE,"send down load settle file fail!!"));
	}
}


void CDownLoadSettleFile::SetRetParam()
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

void CDownLoadSettleFile::BuildResp( CHAR** outbuf, INT32& outlen )
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

void CDownLoadSettleFile::LogProcess()
{
}


