/*
 * CDownLoadCheckBill.cpp
 *
 *  Created on: 2017年7月19日
 *      Author: hawrkchen
 */

#include "CDownLoadCheckBill.h"
#include "CCurlClient.h"
#include "tinyxml2.h"
#include "gzip.h"
//#include "gzipbase64.h"

INT32 CDownLoadCheckBill::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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

	    CallPayGate2GetFactorID();

	    pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBusiConf();

	    for(auto factor_iter : factor_vec)
	    {
	    	string strResBody;

	    	if(m_InParams["pay_channel"] == "SZPF")  //深圳浦发
	    	{
		    	CreateMsgBody(factor_iter);

		    	SendMsgToBank(factor_iter,strResBody);
	    	}
	    	if(m_InParams["pay_channel"] == "GZPF"||m_InParams["pay_channel"] == "SZCIB"||m_InParams["pay_channel"] == "ECITIC")  //广州浦发 &兴业&中信
	    	{
	    		StringMap inparamMap;
	    		CreateSwiftMsgBody(inparamMap,factor_iter);

	    		SendMsgToSwift(inparamMap,factor_iter,strResBody);
	    	}
			if(m_InParams["pay_channel"] == "FJHXBANK")  //福建海峡银行
			{
				GetSpeedPosBill(factor_iter);
			}

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
void CDownLoadCheckBill::FillReq( NameValueMap& mapInput)
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

void CDownLoadCheckBill::CheckInput()
{

	if(m_InParams["ver"].empty() || m_InParams["ver"] != VER)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("bill_date", m_InParams["bill_date"], 1, 10, true);  //对账单日期
	Check::CheckStrParam("pay_channel", m_InParams["pay_channel"], 1, 20, true);  //对账通道


}

void CDownLoadCheckBill::CallPayGate2GetFactorID()
{
	char szRecvBuff[1024*10] = {0};
	factor_vec.clear();
	StringMap paramMap;
	StringMap recvMap;
	CSocket* paygateSocket = Singleton<CSpeedPosConfig>::GetInstance()->GetPaygateServerSocket();
	//调用网关服务 ，获取代理商ID
	paramMap.insert(StringMap::value_type("ver", "1"));
	paramMap.insert(StringMap::value_type("cmd","5021"));
	paramMap.insert(StringMap::value_type("version","1.0"));

	JsonMap  bizCJsonMap;
	bizCJsonMap.insert(JsonMap::value_type(JsonType("dml_type"), "ALL"));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("pay_channel_id"),m_InParams["pay_channel"]));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("group_by"),"Fchannel_factor_id"));
	std::string bizContent = JsonUtil::objectToString(bizCJsonMap);
	paramMap.insert(std::make_pair("biz_content", bizContent));

	paygateSocket->SendAndRecvLineEx(paramMap,szRecvBuff,sizeof(szRecvBuff),"\r\n");
	//CDEBUG_LOG("recv Msg [%s]",szRecvBuff);

	if(NULL == szRecvBuff||strlen(szRecvBuff) == 0)  //
	{
		CDEBUG_LOG("get factor_id fail !!,err_msg[%s]",recvMap["retmsg"].c_str());
		throw(CTrsExp(ERR_DOWNLOAD_BILL,"get factor_id fail!!"));
	}

	cJSON* root = cJSON_Parse(szRecvBuff);
	if (0 != strcmp(cJSON_GetObjectItem(root, "ret_code")->valuestring, "0"))
	{
		CERROR_LOG("ret_code:[%s] ret_msg:[%s]\n", cJSON_GetObjectItem(root, "ret_code")->valuestring, cJSON_GetObjectItem(root, "ret_msg")->valuestring);
		throw (CTrsExp(ERR_CALL_PAYGATE_SERV, cJSON_GetObjectItem(root, "ret_msg")->valuestring));
	}

	//嵌套Json
	cJSON * biz_content = cJSON_GetObjectItem(root, "biz_content");
	cJSON * factor_ids = cJSON_GetObjectItem(biz_content, "lists");
	int iTotal = cJSON_GetArraySize(factor_ids);
	CDEBUG_LOG("factor_id list [%d]",iTotal);
	for (int i = 0; i < iTotal; ++i)
	{
		cJSON* channel = cJSON_GetArrayItem(factor_ids, i);
		JsonType obj = JsonUtil::json2obj(channel);
		JsonMap jMap = obj.toMap();

		StringMap factorMap;
		factorMap["factor_id"] = jMap["channel_factor_id"].toString();
		factor_vec.push_back(factorMap["factor_id"]);
	}

	for(auto iter:factor_vec)
	{
		CDEBUG_LOG("factor_id = %s",iter.c_str());
	}
}

void CDownLoadCheckBill::CreateMsgBody(const string& factor_id)
{
	std::string szResBody = "";
	StringMap paramMap;
	paramMap.insert(std::make_pair("requestNo", GetSysTimeUsecEx(time(NULL))));
	paramMap.insert(std::make_pair("version", BANK_VERSION));
	paramMap.insert(std::make_pair("transId", BANK_TRANSID));
	paramMap.insert(std::make_pair("agentId", factor_id));
	paramMap.insert(std::make_pair("orderDate", m_InParams["bill_date"]));

	m_reqMsg.clear();
	Map2Str(paramMap,m_reqMsg);

	CDEBUG_LOG("reqSignSrc=[%s]\n", m_reqMsg.c_str());

	m_sendUrl = pBillBusConfig->BusiConfig.m_spdb_base_url + DOMAIN_URL;
	CDEBUG_LOG("apiUrl=[%s]\n", m_sendUrl.c_str());
}

void CDownLoadCheckBill::CreateSwiftMsgBody(StringMap& paramMap,const string& factor_id)
{
	time_t tNow = time(NULL);
	std::string strNonceStr = toString(tNow);

	std::string szResBody = "";
	//StringMap paramMap;
	//test
	paramMap.insert(std::make_pair("service", SERVICE_MCH_TYPE));
	paramMap.insert(std::make_pair("mch_id", "7551000001"));

	//proc
//	paramMap.insert(std::make_pair("service", SERVICE_AGENT_TYPE));
//	paramMap.insert(std::make_pair("mch_id", factor_id));

	paramMap.insert(std::make_pair("version", SWIFT_VERSION));
	paramMap.insert(std::make_pair("bill_date", m_InParams["bill_date"]));
	paramMap.insert(std::make_pair("bill_type", SWIFT_BILL_TYPE));
	paramMap.insert(std::make_pair("nonce_str", strNonceStr));

	m_reqMsg.clear();
	Map2Str(paramMap,m_reqMsg);

	CDEBUG_LOG("reqSignSrc=[%s]\n", m_reqMsg.c_str());

	m_sendUrl = pBillBusConfig->BusiConfig.m_swiftpass_url;
	CDEBUG_LOG("apiUrl=[%s]\n", m_sendUrl.c_str());
}

string CDownLoadCheckBill::CreateRSASign(const string& factor_id, const string& plainText)
{
	int iRet = 0;

	std::string szSignStr("");

	NameValueMap AuthReq;
	AuthReq.insert(std::make_pair("ver", "1.0"));
	//调用签名接口
	AuthReq.insert(std::make_pair("cmd", "1004"));

	JsonMap  bizCJsonMap;
	bizCJsonMap.insert(JsonMap::value_type(JsonType("pay_channel_id"), JsonType(m_InParams["pay_channel"])));  //CHINACARDPOS

	bizCJsonMap.insert(JsonMap::value_type(JsonType("type"), JsonType("channel")));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("channel_factor_id"), JsonType(factor_id)));

	bizCJsonMap.insert(JsonMap::value_type(JsonType("src_str"), JsonType(plainText)));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("digest_type"), JsonType("SHA1")));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("rsa_func_type"), JsonType("2")));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("pay_type"), JsonType("upay")));

	std::string bizContent = JsonUtil::objectToString(bizCJsonMap);

	AuthReq.insert(std::make_pair("biz_content", bizContent));

	CSocket *pSocket = Singleton<CSpeedPosConfig>::GetInstance()->GetAuthServerSocket();

	//CDEBUG_LOG("Send to sign Msg =[%s]",bizContent.c_str());

	char szRsp[1024 * 10] = { 0 };
	char szSign[1024 * 10] = { 0 };

	iRet = pSocket->SendAndRecvLineEx(AuthReq, szRsp, 1024 * 10, "\r\n");
	if(iRet < 0)
	{
		CDEBUG_LOG("CreateSign fail !!,err_msg[%s]");
		throw(CTrsExp(ERR_CALL_AUTH_SERV,"CreateSign fail!!"));
	}
	//pSocket->Close();

	//CDEBUG_LOG("szRsp=[%s]\n", szRsp);
	cJSON* root = cJSON_Parse(szRsp);
	if (0 != strcmp(cJSON_GetObjectItem(root, "ret_code")->valuestring, "0")){
		CERROR_LOG("ret_code:[%s] ret_msg:[%s]\n", cJSON_GetObjectItem(root, "ret_code")->valuestring, cJSON_GetObjectItem(root, "ret_msg")->valuestring);
		throw (CTrsExp(ERR_CALL_AUTH_SERV, cJSON_GetObjectItem(root, "ret_msg")->valuestring));
	}
	std::string rspSign = "";

	cJSON* bizRspJson = cJSON_GetObjectItem(root, "biz_content");

	JsonType bizJType = JsonUtil::json2obj(bizRspJson);

	JsonMap rspMap = bizJType.toMap();

	rspSign = GETJSONVALUE("sign", rspMap);

	clib_urlencode_comm(rspSign.c_str(), szSign, 1024 * 10);

	szSignStr = szSign;


	return szSignStr;
}

string CDownLoadCheckBill::CreateMD5Sign(const string& factor_id, const string& plainText)
{
	int iRet = 0;

	std::string szSignStr("");

	NameValueMap AuthReq;
	AuthReq.insert(std::make_pair("ver", "1.0"));
	//调用签名接口
	AuthReq.insert(std::make_pair("cmd", "1002"));  //MD5 签名生成

	JsonMap  bizCJsonMap;

//	bizCJsonMap.insert(JsonMap::value_type(JsonType("type"), JsonType("channel")));
//	bizCJsonMap.insert(JsonMap::value_type(JsonType("src_str"), JsonType(plainText)));
//	bizCJsonMap.insert(JsonMap::value_type(JsonType("pay_channel_id"), JsonType(m_InParams["pay_channel"])));  //
//	bizCJsonMap.insert(JsonMap::value_type(JsonType("channel_factor_id"), JsonType(factor_id)));
//	bizCJsonMap.insert(JsonMap::value_type(JsonType("pay_type"), JsonType("upay")));
//

	bizCJsonMap.insert(JsonMap::value_type(JsonType("type"), JsonType("channel_mch")));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("src_str"), JsonType(plainText)));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("mch_id"), JsonType("10000102")));  // 测试环境用账号
	//bizCJsonMap.insert(JsonMap::value_type(JsonType("mch_id"), JsonType("10000015")));  //开发环境用账号
	bizCJsonMap.insert(JsonMap::value_type(JsonType("pay_channel_id"), JsonType("GZPF")));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("pay_type"), JsonType("upay")));

	std::string bizContent = JsonUtil::objectToString(bizCJsonMap);

	AuthReq.insert(std::make_pair("biz_content", bizContent));

	CSocket *pSocket = Singleton<CSpeedPosConfig>::GetInstance()->GetAuthServerSocket();

	CDEBUG_LOG("Send to Auth sign Msg =[%s]",bizContent.c_str());

	char szRsp[1024 * 10] = { 0 };
	char szSign[1024 * 10] = { 0 };

	iRet = pSocket->SendAndRecvLineEx(AuthReq, szRsp, 1024 * 10, "\r\n");
	if(iRet < 0)
	{
		CDEBUG_LOG("CreateSign fail !!,err_msg[%s]");
		throw(CTrsExp(ERR_CALL_AUTH_SERV,"CreateSign fail!!"));
	}
	//pSocket->Close();

	CDEBUG_LOG("Auth sign szRsp=[%s]\n", szRsp);
	cJSON* root = cJSON_Parse(szRsp);
	if (0 != strcmp(cJSON_GetObjectItem(root, "ret_code")->valuestring, "0")){
		CERROR_LOG("ret_code:[%s] ret_msg:[%s]\n", cJSON_GetObjectItem(root, "ret_code")->valuestring, cJSON_GetObjectItem(root, "ret_msg")->valuestring);
		throw (CTrsExp(ERR_CALL_AUTH_SERV, cJSON_GetObjectItem(root, "ret_msg")->valuestring));
	}
	std::string rspSign = "";

	cJSON* bizRspJson = cJSON_GetObjectItem(root, "biz_content");

	JsonType bizJType = JsonUtil::json2obj(bizRspJson);

	JsonMap rspMap = bizJType.toMap();

	rspSign = GETJSONVALUE("sign", rspMap);

	clib_urlencode_comm(rspSign.c_str(), szSign, 1024 * 10);

	szSignStr = szSign;


	return szSignStr;
}

void CDownLoadCheckBill::SendMsgToBank(const std::string& factor_id, std::string& szResBody)
{
	BEGIN_LOG(__func__);

	CRouteBillBusiConf* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBusiConf();

	struct timeval stStart;
	struct timeval stEnd;
	StringMap urlRspMap;

	CurlClient curl;

	std::string reqSign = "", reqMsg = "", szResHead = "";

	curl.SetSSLVerify(false);

	curl.SetTimeOut(20);


	reqSign = CreateRSASign(factor_id, m_reqMsg);

	CDEBUG_LOG("sign = [%s]\n", reqSign.c_str());

	reqMsg = m_reqMsg + "&signature=" + reqSign;

	CDEBUG_LOG("szReqUrl=[%s], reqMsg=[%s]\n", m_sendUrl.c_str(), reqMsg.c_str());
	gettimeofday(&stStart, NULL);
	if (!curl.Post(m_sendUrl, reqMsg, szResHead, szResBody))
	{
		CERROR_LOG("curl post fail err_code=[%d] err_msg[%s] szResHead=[%s],szResBody=[%s]",
			curl.GetErrCode(), curl.GetErrInfo().c_str(), szResHead.c_str(), szResBody.c_str());
		throw (CTrsExp(curl.GetErrCode(), curl.GetErrInfo()));
	}
	gettimeofday(&stEnd, NULL);
	int iUsedTime = CalDiff(stStart, stEnd);
	CDEBUG_LOG("szResHead=[%s], szResBody=[%s] iUsedTime[%d] \n", szResHead.c_str(), szResBody.c_str(), iUsedTime);

	Kv2Map(szResBody, urlRspMap);

	if(urlRspMap["respCode"] != BANK_RET_SUCCESS)
	{
		CERROR_LOG("bank ret :err_code:[%s] err_code_msg:[%s]\n", urlRspMap["respCode"].c_str(), urlRspMap["respDesc"].c_str());
		throw (CTrsExp(ERR_GET_BANK_RESP, urlRspMap["respDesc"]));
	}

	CDEBUG_LOG("file_detail :[%s],fileName = [%s]",urlRspMap["fileContentDetail"].c_str(),urlRspMap["fileName"].c_str());
	//TODO: test
	//base64 decode

	//urlRspMap["fileContentDetail"] = "H4sIAAAAAAAAAK2SS0oDQRCG94J38AAdqGe/PI8nMBfIQkEUXJigO3UlqDgrEVT0MjOTeAurNW4mDYnoYpoZ+q/pr78qAgwQURwjiAADAElScIAQXff20L5f90d386tJP23alwsXLbCsQaBoRYlDUBUM3mrQld0RxBHKDkKmmEEcgv3wa7WHWIgw+mpyPr1sn28+bk8WzWR3e4v+gicqIJqAgh2YSOIQT7JKOZQLmKOUlnhWnria7B+PF82smx12Z2vwkJZ43cF9+3renT7VBDKKcSW0b/YqRelAC2MWqggMrNXk5gI3IkRITJG9FzKbwDR0iBlSZl4ltFulevIfsETtnWKQyGUAi7iV1tpIGQyy+15/WkuaqslftDatm7zCpUFZUSFCgJW+hiwxsx9aU1D2UE2O9/bHhvUJVWYW9LADAAA=";
	string decode_msg = tars::TC_Base64::decode(urlRspMap["fileContentDetail"]);
	//CDEBUG_LOG("decode_msg = [%s]",decode_msg.c_str());

	//gzip depress
//	unsigned char *buf = NULL;
//	buf  = new unsigned char[decode_msg.length() + 1];


	unsigned char buf[10240] = {0};
	unsigned long len;
	gzdecompress((unsigned char*)decode_msg.c_str(),(unsigned long)decode_msg.length(),buf,&len);
	CDEBUG_LOG("out:[%s]",buf);

	string file_content = (char*)buf;

//	string file_content = decode_gzipbase64(urlRspMap["fileContentDetail"]);
//	CDEBUG_LOG("file_content = [%s]",file_content.c_str());

	//文件路径+ 通道名 + 通道代理ID + 日期
	string file_name = pBillBusConfig->BusiConfig.m_spdb_bill_path + m_InParams["pay_channel"] + "_"
			+ factor_id + "_" + m_InParams["bill_date"];
	CDEBUG_LOG("file_name = [%s]",file_name.c_str());
	tars::TC_File::save2file(file_name,file_content);

//	if(tars::TC_File::getFileSize(file_name) == 0)
//	{
//		CDEBUG_LOG("no data");
//	}
//	else
//	{
//		CDEBUG_LOG("file_size = [%d]",tars::TC_File::getFileSize(file_name));
//	}

	//delete buf;
}

void CDownLoadCheckBill::SendMsgToSwift(const StringMap& paramMap,const std::string& factor_id, std::string& szResBody)
{
	BEGIN_LOG(__func__);

	CRouteBillBusiConf* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBusiConf();

	struct timeval stStart;
	struct timeval stEnd;
	StringMap urlRspMap;

	CurlClient curl;
	tinyxml2::XMLDocument doc_wx;
	tinyxml2::XMLPrinter oPrinter;

	std::string reqSign = "", reqMsg = "", szResHead = "";

	curl.SetSSLVerify(false);

	curl.SetTimeOut(20);


	reqSign = CreateMD5Sign(factor_id, m_reqMsg);

	CDEBUG_LOG("sign = [%s]\n", reqSign.c_str());

	tinyxml2::XMLNode* pXmlNode = doc_wx.InsertEndChild(doc_wx.NewElement("xml"));
	if (!pXmlNode)
	{
		CDEBUG_LOG("ERROR:create xml element node fail!!");
		throw (CTrsExp(ERR_PARSE_XML_FAIL, "ERROR:create xml element node fail!!"));
	}

	for(auto iter : paramMap)
	{
		SetOneFieldToXml(&doc_wx, pXmlNode,iter.first.c_str(),iter.second.c_str(),false);
	}

	SetOneFieldToXml(&doc_wx, pXmlNode, "sign", reqSign.c_str(), false);
	doc_wx.Accept(&oPrinter);
	reqMsg = oPrinter.CStr();

	CDEBUG_LOG("szReqUrl=[%s]\n, reqMsg=[%s]\n", m_sendUrl.c_str(), reqMsg.c_str());
	gettimeofday(&stStart, NULL);
	if (!curl.Post(m_sendUrl, reqMsg, szResHead, szResBody))
	{
		CERROR_LOG("curl post fail err_code=[%d] err_msg[%s] szResHead=[%s],szResBody=[%s]",
			curl.GetErrCode(), curl.GetErrInfo().c_str(), szResHead.c_str(), szResBody.c_str());
		throw (CTrsExp(curl.GetErrCode(), curl.GetErrInfo()));
	}
	gettimeofday(&stEnd, NULL);
	int iUsedTime = CalDiff(stStart, stEnd);
	CDEBUG_LOG("szResHead=[%s], szResBody=[%s] iUsedTime[%d] \n", szResHead.c_str(), szResBody.c_str(), iUsedTime);

	string file_name = pBillBusConfig->BusiConfig.m_spdb_bill_path + m_InParams["pay_channel"] + "_"
			+ factor_id + "_" + m_InParams["bill_date"];
	CDEBUG_LOG("swift file_name = [%s]",file_name.c_str());
	tars::TC_File::save2file(file_name,szResBody);

}

void CDownLoadCheckBill::GetSpeedPosBill(const std::string& factor_id)
{
	CDEBUG_LOG("Begin ...");

	string szRemotePath = pBillBusConfig->BusiConfig.m_speedpos_remote_path + "/" +factor_id + "/webbill";

	char szCmd[512] = { 0 };
	memset(szCmd,0x00,sizeof(szCmd));
	
	snprintf(szCmd, sizeof(szCmd), "sh %s %s %s %s %s %s %s %s %s",
			pBillBusConfig->BusiConfig.m_get_speedpos_bill_shell.c_str(), 
			pBillBusConfig->BusiConfig.m_speedpos_sftp_ip.c_str(), 
			pBillBusConfig->BusiConfig.m_speedpos_sftp_port.c_str(),
			pBillBusConfig->BusiConfig.m_speedpos_sftp_user.c_str(), 
			pBillBusConfig->BusiConfig.m_speedpos_sftp_pwd.c_str(),
			factor_id.c_str(),
			m_InParams["bill_date"].c_str(),
			szRemotePath.c_str(),
			pBillBusConfig->BusiConfig.m_spdb_bill_path.c_str());
	CDEBUG_LOG("sh command szCmd = [%s]",szCmd);
	system(szCmd);
}



void CDownLoadCheckBill::SetRetParam()
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

void CDownLoadCheckBill::BuildResp( CHAR** outbuf, INT32& outlen )
{
	CDEBUG_LOG("Begin ...");

    CHAR szResp[ MAX_RESP_LEN ];
	JsonMap jsonRsp;

	if(m_RetMap["err_code"].empty())
	{
		m_RetMap["err_code"] = "0";
		m_RetMap["err_msg"]  = "success";
	}
//	else
//    {
//        Singleton<CSpeedPosConfig>::GetInstance()->GetBusiConf()->ReadErrCodeFromFile(m_RetMap);
//    }

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

void CDownLoadCheckBill::LogProcess()
{
}



