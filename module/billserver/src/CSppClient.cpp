/*
 * CSppClient.cpp
 *
 *  Created on: 2010-5-20
 *      Author: rogeryang
 */
#include "network.h"
#include "id_protocol.h"
#include "url_protocol.h"
#include "../Comm/tools.h"
#include "l5_wrap.h"
#include "CSppClient.h"
#include "log/clog.h"

CSppClient::CSppClient()
{
    // TODO Auto-generated constructor stub
}

CSppClient::~CSppClient()
{
    // TODO Auto-generated destructor stub
}


INT32 CSppClient::Init(const CommServer & stServer)
{
    m_cSocket.SetServer( stServer );   //trade_server
    return 0;
}



INT32 CSppClient::CallOrderQuery(const std::string& strOrderNo, SOrderInfoRsp& rsp)
{

	CDEBUG_LOG("into CallOrderQuery");
	INT32 iRet = -1;
	Reset();
	char szBuf[2048] = {0};

	NameValueMap mapSend, mapRecv;
	char szTmp[16] = { 0 };
	SET_INT(szTmp, 1);
	mapSend["ver"] = szTmp;

	SET_INT(szTmp, 190);
	mapSend["cmd"] = szTmp;

	SET_INT(szTmp, 1);
	mapSend["src"] = szTmp;

	mapSend["order_no"] = strOrderNo;

	iRet = m_cSocket.SendAndRecv(mapSend, (void*)szBuf,sizeof(szBuf));
	m_cSocket.Close();
//	if (iRet != 0)
//	{
//		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "SendAndRecvLine failed.Ret[%d] Err[%s]",
//			iRet, m_cSocket.GetErrorMessage());
//		CDEBUG_LOG("Error Msg:[%s]",m_szErrMsg);
//		return -20;
//	}
	if(NULL != szBuf)
	{
		cJSON* root = cJSON_Parse(szBuf);
		cJSON * ret_code = cJSON_GetObjectItem(root, "retcode");
		if(NULL != ret_code)
		{
			CDEBUG_LOG("ret_code = [%d]",ret_code->valueint);
		}
		cJSON * order_status = cJSON_GetObjectItem(root, "order_status");
		if(NULL != order_status)
		{
			CDEBUG_LOG("order_status:[%d]",order_status->valueint);
			if(order_status->valueint == 1)
				iRet = 0;
		}
	}

	return iRet;
}


INT32 CSppClient::SendBillNotify(StringMap&parammap,SOrderInfoRsp& rsp)
{
	INT32 iRet = 0;
	Reset();

	NameValueMap mapSend, mapRecv;

//	char szTmp[16] = { 0 };
//	SET_INT(szTmp, 1);
//	mapSend["ver"] = szTmp;
//
//	SET_INT(szTmp, 190);
//	mapSend["cmd"] = szTmp;
//
//	SET_INT(szTmp, 1);
//	mapSend["src"] = szTmp;


	mapSend["ops"] 			= parammap["ops"];
	mapSend["bkname"]   	= parammap["bkname"];
	mapSend["bmid"] 		= parammap["bmid"];
	mapSend["paychannel"] 	= parammap["paychannel"];

	//CDEBUG_LOG("begin to send,ops = [%s],bkname=[%s]",mapSend["ops"].c_str(),mapSend["bkname"].c_str());

	iRet = m_cSocket.SendAndRecvLine(mapSend, mapRecv, "\r\n");
	CDEBUG_LOG("iRet = [%d]",iRet);
	m_cSocket.Close();
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "SendAndRecvLine failed.Ret[%d] Err[%s]",
			iRet, m_cSocket.GetErrorMessage());
		return -20;
	}


	//INT32 iRetCode = 0;
	FETCH_INT_VALUE(mapRecv, rsp.result.ret_code, "RETCODE", -30, "parse response failed(no retcode)");
	FETCH_STRING_STD_EX_EX(mapRecv, rsp.result.ret_msg, "ERRMSG", "");


	return rsp.result.ret_code;
}
