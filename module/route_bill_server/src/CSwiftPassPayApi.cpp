/*
 * CRedisClient.cpp
 *
 *  Created on: 2012-12-12
 *      Author: sangechen
 */


#include "CSwiftPassPayApi.h"
#include "log/clog.h"
#include "urlcodec.h"

#define  SWIFT_PASS_VERSION		"2.0"
//#define  SWIFT_BASE_URL			"https://pay.swiftpass.cn/pay/gateway"

#define  SWIFT_SERVICE_QUERY_CMD			"unified.trade.query"
#define  SWIFT_SERVICE_REFUND_CMD			"unified.trade.refund"
#define  SWIFT_SERVICE_REFUND_QUERY_CMD		"unified.trade.refundquery"
#define  SWIFT_SERVICE_CLOSE_CMD			"unified.trade.close"


struct cmp_sstr
{
	bool operator()(const char* a, const char* b)
	{
		return strcmp(a, b) < 0;
	}
};

CSwiftPassPayApi::CSwiftPassPayApi()
{
	base_url_ = Singleton<CRoutePayGateConfig>::GetInstance()->GetBusConfig()->GetBaseUrl(SWIFT_PASS_API_PAY_CHANNEL);
}

CSwiftPassPayApi::~CSwiftPassPayApi()
{
    
}

void CSwiftPassPayApi::Reset()
{
	
}




int CSwiftPassPayApi::AddShop(StringMap& inReq, StringMap& outRsp)
{
	BEGIN_LOG(__func__);

	return 0;
}

int CSwiftPassPayApi::SendMsgToApi(const std::string& mchId, 
		const std::string& payChannelId, const std::string& apiUrl, 
		StringMap& paramMap, std::string& szResBody)
{
	BEGIN_LOG(__func__);
	std::string reqSign = "", reqMsg = "", szResHead = "";

	CurlClient curl;

	tinyxml2::XMLDocument doc_wx;

	tinyxml2::XMLPrinter oPrinter;

	StringMap::const_iterator iter;
	std::string content = "";
	for (iter = paramMap.begin();
		iter != paramMap.end(); ++iter) {
		if (!content.empty()) {
			content.push_back('&');
		}
		content.append(iter->first);
		content.push_back('=');
		content.append(iter->second);
	}
	std::string szSign = CreateAgentSign(mchId, payChannelId, content);
	
	tinyxml2::XMLNode* pXmlNode = doc_wx.InsertEndChild(doc_wx.NewElement("xml"));
	if (!pXmlNode) return ERR_CREATE_XMLNode;
	for (iter = paramMap.begin();
		iter != paramMap.end(); ++iter)
	{
		SetOneFieldToXml(&doc_wx, pXmlNode, iter->first.c_str(), iter->second.c_str(), false);
	}
	SetOneFieldToXml(&doc_wx, pXmlNode, "sign", szSign, false);
	doc_wx.Accept(&oPrinter);
	reqMsg = oPrinter.CStr();
	
	curl.SetSSLVerify(false);

	curl.SetTimeOut(20);

	CDEBUG_LOG("apiUrl=[%s], reqMsg=[%s]\n", apiUrl.c_str(), reqMsg.c_str());
	gettimeofday(&stStart, NULL);
	if (!curl.Post(apiUrl, reqMsg, szResHead, szResBody))
	{
		CERROR_LOG("curl post fail err_code=[%d] err_msg[%s] szResHead=[%s],szResBody=[%s]",
			curl.GetErrCode(), curl.GetErrInfo().c_str(), szResHead.c_str(), szResBody.c_str());
		throw CException(curl.GetErrCode(), curl.GetErrInfo(), __FILE__, __LINE__);
	}
	gettimeofday(&stEnd, NULL);
	int iUsedTime = CalDiff(stStart, stEnd);
	CDEBUG_LOG("szResHead=[%s], szResBody=[%s] iUsedTime[%d] \n", szResHead.c_str(), szResBody.c_str(), iUsedTime);

	return 0;
}

string CSwiftPassPayApi::CreateAgentSign(const string& idStr, const std::string& payChannelId, const string& plainText)
{
	
	int iRet = 0;

	std::string szSignStr("");

	NameValueMap AuthReq;
	AuthReq.insert(std::make_pair("ver", "1.0"));
	//调用签名接口
	AuthReq.insert(std::make_pair("cmd", "1002"));

	JsonMap  bizCJsonMap;
	bizCJsonMap.insert(JsonMap::value_type(JsonType("type"), JsonType("channel_mch")));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("src_str"), JsonType(plainText)));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("pay_channel_id"), JsonType(payChannelId)));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("mch_id"), JsonType(idStr)));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("pay_type"), JsonType("upay")));

	std::string bizContent = JsonUtil::objectToString(bizCJsonMap);

	AuthReq.insert(std::make_pair("biz_content", bizContent));

	CSocket *pSocket = Singleton<CRoutePayGateConfig>::GetInstance()->GetAuthSocket();

	char szRsp[1024 * 10] = { 0 };
	char szSign[1024 * 10] = { 0 };

	iRet = pSocket->SendAndRecvLineEx(AuthReq, szRsp, 1024 * 10, "\r\n");
	if (0 > iRet)  throw CException(ERR_SYSERR, errMap[ERR_SYSERR], __FILE__, __LINE__);

	pSocket->Close();

	CDEBUG_LOG("szRsp=[%s]\n", szRsp);
	cJSON* root = cJSON_Parse(szRsp);
	if (0 != strcmp(cJSON_GetObjectItem(root, "ret_code")->valuestring, "0")){
		CERROR_LOG("ret_code:[%s] ret_msg:[%s]\n", cJSON_GetObjectItem(root, "ret_code")->valuestring, cJSON_GetObjectItem(root, "ret_msg")->valuestring);
		throw CException(ERR_CALL_AUTH_SERVER_API, cJSON_GetObjectItem(root, "ret_msg")->valuestring, __FILE__, __LINE__);
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


int CSwiftPassPayApi::Unifiedorder(StringMap& inReq, StringMap& outRsp)
{
	BEGIN_LOG(__func__);

	std::string szResBody = "";
	StringMap param;
	param.insert(StringMap::value_type("service", inReq["pay_service"]));
	param.insert(StringMap::value_type("version", SWIFT_PASS_VERSION));
	param.insert(StringMap::value_type("mch_id", inReq["channel_mch_id"]));
	param.insert(StringMap::value_type("out_trade_no", inReq["order_no"]));
	param.insert(StringMap::value_type("body", inReq["body"]));
	param.insert(StringMap::value_type("total_fee", inReq["total_fee"]));
	param.insert(StringMap::value_type("mch_create_ip", inReq["spbill_create_ip"]));
	if (inReq["pay_type"] == TRADE_TYPE_MICROPAY)
	{
		param.insert(std::make_pair("auth_code", inReq["auth_code"]));
	}
	else if (inReq["pay_type"] == TRADE_TYPE_JSAPI)
	{
		if (!inReq["sub_openid"].empty())param.insert(std::make_pair("sub_openid", inReq["sub_openid"]));
		if (!inReq["sub_appid"].empty())param.insert(std::make_pair("sub_appid", inReq["sub_appid"]));
	}
	param.insert(StringMap::value_type("notify_url", inReq["notify_url"]));
	param.insert(StringMap::value_type("nonce_str", toString(time(NULL))));

	param.insert(StringMap::value_type("is_raw", "1"));
	if (!inReq["attach"].empty())param.insert(StringMap::value_type("attach", inReq["attach"]));
	if (!inReq["device_info"].empty())param.insert(StringMap::value_type("device_info", inReq["device_info"]));
	if (!inReq["limit_credit_pay"].empty()) param.insert(StringMap::value_type("limit_credit_pay", inReq["limit_credit_pay"]));

	SendMsgToApi(inReq["mch_id"], inReq["pay_channel_id"], inReq["api_url"], param, szResBody);

	tinyxml2::XMLDocument wx_rsp_doc;
	if (tinyxml2::XML_SUCCESS != wx_rsp_doc.Parse(szResBody.c_str(), szResBody.size()))
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_PASE_XML, errMap[ERR_PASE_XML].c_str());
		return ERR_PASE_XML;
	}

	tinyxml2::XMLElement * xmlElement = wx_rsp_doc.FirstChildElement("xml");
	if (NULL == xmlElement)
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_CREATE_XMLNode, errMap[ERR_CREATE_XMLNode].c_str());
		return ERR_CREATE_XMLNode;
	}

	const char* prsp_status = GetXmlField(xmlElement, "status");
	const char* prsp_message = GetXmlField(xmlElement, "message");
	if (!(prsp_status && strncmp(prsp_status, "0", 1) == 0))
	{
		std::string  ret_code = prsp_status ? prsp_status : "";
		std::string ret_msg = prsp_message ? prsp_message : "";
		
		CERROR_LOG("ret_code [%s] ret_msg [%s] \n", ret_code.c_str(), ret_msg.c_str());
		throw CException(ERR_CALL_UNIFIEDORDER_API_REQ, ret_msg.c_str(), __FILE__, __LINE__);
	}
	else
	{
		const char* prsp_result_code = GetXmlField(xmlElement, "result_code");	
		if (!(prsp_result_code && strncmp(prsp_result_code, "0", 1) == 0))
		{
			const char* prsp_err_code = GetXmlField(xmlElement, "err_code");
			const char* prsp_err_msg = GetXmlField(xmlElement, "err_msg");
			CERROR_LOG("err_code : [%s] err_msg : [%s] \n", prsp_err_code, prsp_err_msg);
			throw CException(ERR_CALL_UNIFIEDORDER_API_REQ, prsp_err_msg, __FILE__, __LINE__);
		}
	}
	if (!CheckAgentSign(&wx_rsp_doc, inReq["mch_id"], inReq["pay_channel_id"]))
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_SIGN_CHECK, errMap[ERR_SIGN_CHECK].c_str());
		return ERR_SIGN_CHECK;
	}

	if (inReq["pay_type"] == TRADE_TYPE_NATIVE)
	{
		const char* prsp_code_url = GetXmlField(xmlElement, "code_url");
		outRsp.insert(std::make_pair("code_url", prsp_code_url));
		outRsp.insert(std::make_pair("order_no", inReq["order_no"]));
		outRsp.insert(std::make_pair("total_fee", inReq["total_fee"]));
	}
	else if (inReq["pay_type"] == TRADE_TYPE_MICROPAY)
	{
		const char* prsp_fee_type = GetXmlField(xmlElement, "fee_type");
		const char* prsp_out_transaction_id = GetXmlField(xmlElement, "out_transaction_id");
		const char* prsp_total_fee = GetXmlField(xmlElement, "total_fee");
		const char* prsp_transaction_id = GetXmlField(xmlElement, "transaction_id");
		const char* prsp_order_no = GetXmlField(xmlElement, "out_trade_no");

		const char* prsp_pay_result = GetXmlField(xmlElement, "pay_result");

		if (prsp_fee_type)outRsp.insert(std::make_pair("fee_type", prsp_fee_type));
		if (prsp_out_transaction_id)outRsp.insert(std::make_pair("out_transaction_id", prsp_out_transaction_id));
		if (prsp_total_fee)outRsp.insert(std::make_pair("total_fee", prsp_total_fee));
		if (prsp_transaction_id)outRsp.insert(std::make_pair("transaction_id", prsp_transaction_id));
		if (prsp_order_no)outRsp.insert(std::make_pair("order_no", prsp_order_no));
	}
	else if (inReq["pay_type"] == TRADE_TYPE_JSAPI)
	{
		const char* prsp_pay_info = GetXmlField(xmlElement, "pay_info");
		if (prsp_pay_info)outRsp.insert(std::make_pair("prepay_id", prsp_pay_info));
		outRsp.insert(std::make_pair("order_no", inReq["order_no"]));
		outRsp.insert(std::make_pair("total_fee", inReq["total_fee"]));
	}
	return 0;
}

bool CSwiftPassPayApi::CheckAgentSign(const tinyxml2::XMLDocument* pDoc, 
				const std::string& mchId, const std::string& payChannelId)
{
	char szRsp[1024 * 10] = { 0 };

	const tinyxml2::XMLElement * xmlElement = pDoc->FirstChildElement("xml");
	if (xmlElement == NULL)
	{
		return false;
	}

	std::map<const char*, const char*, cmp_sstr> params;
	const char* pszSign = NULL;

	for (const tinyxml2::XMLElement* child = xmlElement->FirstChildElement();
		child != NULL; child = child->NextSiblingElement())
	{
		if (strcmp("sign", child->Name()) != 0)
		{
			const char* pszValue = child->GetText();
			if (pszValue && pszValue[0] != '\0')
			{
				params.insert(std::make_pair(child->Name(), pszValue));
			}
		}
		else
		{
			pszSign = child->GetText();
		}
	}

	std::stringstream ss;
	std::map<const char*, const char*, cmp_sstr>::iterator it = params.begin();
	if (it != params.end())
	{
		ss << it->first << "=" << it->second;
		++it;
	}

	for (; it != params.end(); ++it)
	{
		ss << "&" << it->first << "=" << it->second;
	}
	
	std::string urlCode = urlEncode(ss.str());

	NameValueMap AuthReq;
	AuthReq.insert(std::make_pair("ver", "1.0"));
	//调用签名接口
	AuthReq.insert(std::make_pair("cmd", "1003"));

	JsonMap  bizCJsonMap;
	bizCJsonMap.insert(JsonMap::value_type(JsonType("type"), JsonType("channel_mch")));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("src_str"), JsonType(urlCode)));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("pay_channel_id"), JsonType(payChannelId)));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("mch_id"), JsonType(mchId)));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("pay_type"), JsonType("upay")));

	bizCJsonMap.insert(JsonMap::value_type(JsonType("sign"), JsonType(pszSign)));

	std::string bizContent = JsonUtil::objectToString(bizCJsonMap);

	AuthReq.insert(std::make_pair("biz_content", bizContent));

	CSocket *pSocket = Singleton<CRoutePayGateConfig>::GetInstance()->GetAuthSocket();

	pSocket->SendAndRecvLineEx(AuthReq, szRsp, 1024 * 10, "\r\n");
	pSocket->Close();

	CDEBUG_LOG("szRsp=[%s]\n", szRsp);
	cJSON* root = cJSON_Parse(szRsp);
	if (0 != strcmp(cJSON_GetObjectItem(root, "ret_code")->valuestring, "0")){
		CERROR_LOG("ret_code:[%s] ret_msg:[%s]\n", cJSON_GetObjectItem(root, "ret_code")->valuestring, cJSON_GetObjectItem(root, "ret_msg")->valuestring);
		return false;
	}

	return true;
}

int CSwiftPassPayApi::Micropay(StringMap& inReq, StringMap& outRsp)
{
	
	return 0;
}

int CSwiftPassPayApi::OrderQuery(StringMap& inReq, StringMap& outRsp)
{
	BEGIN_LOG(__func__);

	std::string szResBody = "";
	StringMap param;
	param.insert(StringMap::value_type("service", SWIFT_SERVICE_QUERY_CMD));
	param.insert(StringMap::value_type("version", SWIFT_PASS_VERSION));
	param.insert(StringMap::value_type("mch_id", inReq["channel_mch_id"]));
	param.insert(StringMap::value_type("out_trade_no", inReq["order_no"]));
	param.insert(StringMap::value_type("nonce_str", toString(time(NULL))));

	SendMsgToApi(inReq["mch_id"], inReq["pay_channel_id"], base_url_, param, szResBody);

	tinyxml2::XMLDocument wx_rsp_doc;
	if (tinyxml2::XML_SUCCESS != wx_rsp_doc.Parse(szResBody.c_str(), szResBody.size()))
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_PASE_XML, errMap[ERR_PASE_XML].c_str());
		return ERR_PASE_XML;
	}

	tinyxml2::XMLElement * xmlElement = wx_rsp_doc.FirstChildElement("xml");
	if (NULL == xmlElement)
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_CREATE_XMLNode, errMap[ERR_CREATE_XMLNode].c_str());
		return ERR_CREATE_XMLNode;
	}

	const char* prsp_status = GetXmlField(xmlElement, "status");
	const char* prsp_message = GetXmlField(xmlElement, "message");
	if (!(prsp_status && strncmp(prsp_status, "0", 1) == 0))
	{
		std::string  ret_code = prsp_status ? prsp_status : "";
		std::string ret_msg = prsp_message ? prsp_message : "";

		CERROR_LOG("ret_code [%s] ret_msg [%s] \n", ret_code.c_str(), ret_msg.c_str());
		throw CException(ERR_CALL_UNIFIEDORDER_API_REQ, ret_msg.c_str(), __FILE__, __LINE__);
	}
	else
	{
		const char* prsp_result_code = GetXmlField(xmlElement, "result_code");
		if (!(prsp_result_code && strncmp(prsp_result_code, "0", 1) == 0))
		{
			const char* prsp_err_code = GetXmlField(xmlElement, "err_code");
			const char* prsp_err_msg = GetXmlField(xmlElement, "err_msg");
			CERROR_LOG("err_code : [%s] err_msg : [%s] \n", prsp_err_code, prsp_err_msg);
			throw CException(ERR_CALL_UNIFIEDORDER_API_REQ, prsp_err_msg, __FILE__, __LINE__);
		}
	}
	if (!CheckAgentSign(&wx_rsp_doc, inReq["mch_id"], inReq["pay_channel_id"]))
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_SIGN_CHECK, errMap[ERR_SIGN_CHECK].c_str());
		return ERR_SIGN_CHECK;
	}
	const char* prsp_trade_state = GetXmlField(xmlElement, "trade_state");
	const char* prsp_transaction_id = GetXmlField(xmlElement, "transaction_id");
	const char* prsp_out_transaction_id = GetXmlField(xmlElement, "out_transaction_id");
	const char* prsp_order_no = GetXmlField(xmlElement, "out_trade_no");
	const char* prsp_total_fee = GetXmlField(xmlElement, "total_fee");
	const char* prsp_fee_type = GetXmlField(xmlElement, "fee_type");
	const char* prsp_bank_type = GetXmlField(xmlElement, "bank_type");
	const char* prsp_time_end = GetXmlField(xmlElement, "time_end");

	outRsp.insert(std::make_pair("order_no", inReq["order_no"]));
	if (prsp_trade_state) outRsp.insert(std::make_pair("order_status", prsp_trade_state));
	if (prsp_transaction_id) outRsp.insert(std::make_pair("transaction_id", prsp_transaction_id));
	if (prsp_out_transaction_id) outRsp.insert(std::make_pair("out_transaction_id", prsp_out_transaction_id));
	if (prsp_total_fee) outRsp.insert(std::make_pair("total_fee", prsp_total_fee));
	if (prsp_fee_type) outRsp.insert(std::make_pair("fee_type", prsp_fee_type));
	if (prsp_bank_type) outRsp.insert(std::make_pair("bank_type", prsp_bank_type));
	if (prsp_time_end) outRsp.insert(std::make_pair("pay_time", prsp_time_end));
	
	return 0;
}

int CSwiftPassPayApi::Refund(StringMap& inReq, StringMap& outRsp)
{
	BEGIN_LOG(__func__);

	std::string szResBody = "";
	StringMap param;
	param.insert(StringMap::value_type("service", SWIFT_SERVICE_REFUND_CMD));
	param.insert(StringMap::value_type("version", SWIFT_PASS_VERSION));
	param.insert(StringMap::value_type("mch_id", inReq["channel_mch_id"]));
	param.insert(StringMap::value_type("out_trade_no", inReq["order_no"]));
	param.insert(StringMap::value_type("out_refund_no", inReq["refund_no"]));
	param.insert(StringMap::value_type("total_fee", inReq["total_fee"]));
	param.insert(StringMap::value_type("refund_fee", inReq["refund_fee"]));
	param.insert(StringMap::value_type("op_user_id", inReq["channel_mch_id"]));
	param.insert(StringMap::value_type("nonce_str", toString(time(NULL))));

	SendMsgToApi(inReq["mch_id"], inReq["pay_channel_id"], base_url_, param, szResBody);

	tinyxml2::XMLDocument wx_rsp_doc;
	if (tinyxml2::XML_SUCCESS != wx_rsp_doc.Parse(szResBody.c_str(), szResBody.size()))
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_PASE_XML, errMap[ERR_PASE_XML].c_str());
		return ERR_PASE_XML;
	}

	tinyxml2::XMLElement * xmlElement = wx_rsp_doc.FirstChildElement("xml");
	if (NULL == xmlElement)
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_CREATE_XMLNode, errMap[ERR_CREATE_XMLNode].c_str());
		return ERR_CREATE_XMLNode;
	}

	const char* prsp_status = GetXmlField(xmlElement, "status");
	const char* prsp_message = GetXmlField(xmlElement, "message");
	if (!(prsp_status && strncmp(prsp_status, "0", 1) == 0))
	{
		std::string  ret_code = prsp_status ? prsp_status : "";
		std::string ret_msg = prsp_message ? prsp_message : "";

		CERROR_LOG("ret_code [%s] ret_msg [%s] \n", ret_code.c_str(), ret_msg.c_str());
		throw CException(ERR_CALL_UNIFIEDORDER_API_REQ, ret_msg.c_str(), __FILE__, __LINE__);
	}
	else
	{
		const char* prsp_result_code = GetXmlField(xmlElement, "result_code");
		if (!(prsp_result_code && strncmp(prsp_result_code, "0", 1) == 0))
		{
			const char* prsp_err_code = GetXmlField(xmlElement, "err_code");
			const char* prsp_err_msg = GetXmlField(xmlElement, "err_msg");
			CERROR_LOG("err_code : [%s] err_msg : [%s] \n", prsp_err_code, prsp_err_msg);
			throw CException(ERR_CALL_UNIFIEDORDER_API_REQ, prsp_err_msg, __FILE__, __LINE__);
		}
	}
	if (!CheckAgentSign(&wx_rsp_doc, inReq["mch_id"], inReq["pay_channel_id"]))
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_SIGN_CHECK, errMap[ERR_SIGN_CHECK].c_str());
		return ERR_SIGN_CHECK;
	}
	outRsp.insert(std::make_pair("order_no", inReq["order_no"]));

	const char* prsp_transaction_id = GetXmlField(xmlElement, "transaction_id");
	const char* prsp_refund_no = GetXmlField(xmlElement, "out_refund_no");
	const char* prsp_refund_id = GetXmlField(xmlElement, "refund_id");
	const char* prsp_refund_fee = GetXmlField(xmlElement, "refund_fee");

	if (prsp_transaction_id) outRsp.insert(std::make_pair("transaction_id", prsp_transaction_id));
	if (prsp_refund_no) outRsp.insert(std::make_pair("refund_no", prsp_refund_no));
	if (prsp_refund_id) outRsp.insert(std::make_pair("refund_id", prsp_refund_id));
	if (prsp_refund_fee) outRsp.insert(std::make_pair("refund_fee", prsp_refund_fee));

	return 0;
}

int CSwiftPassPayApi::RefundQuery(StringMap& inReq, StringMap& outRsp)
{
	BEGIN_LOG(__func__);

	std::string szResBody = "";
	StringMap param;
	param.insert(StringMap::value_type("service", SWIFT_SERVICE_REFUND_QUERY_CMD));
	param.insert(StringMap::value_type("version", SWIFT_PASS_VERSION));
	param.insert(StringMap::value_type("mch_id", inReq["channel_mch_id"]));
	param.insert(StringMap::value_type("out_refund_no", inReq["refund_no"]));
	param.insert(StringMap::value_type("nonce_str", toString(time(NULL))));
	if (!inReq["order_no"].empty())param.insert(StringMap::value_type("out_trade_no", inReq["order_no"]));

	SendMsgToApi(inReq["mch_id"], inReq["pay_channel_id"], base_url_, param, szResBody);

	tinyxml2::XMLDocument wx_rsp_doc;
	if (tinyxml2::XML_SUCCESS != wx_rsp_doc.Parse(szResBody.c_str(), szResBody.size()))
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_PASE_XML, errMap[ERR_PASE_XML].c_str());
		return ERR_PASE_XML;
	}

	tinyxml2::XMLElement * xmlElement = wx_rsp_doc.FirstChildElement("xml");
	if (NULL == xmlElement)
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_CREATE_XMLNode, errMap[ERR_CREATE_XMLNode].c_str());
		return ERR_CREATE_XMLNode;
	}

	const char* prsp_status = GetXmlField(xmlElement, "status");
	const char* prsp_message = GetXmlField(xmlElement, "message");
	if (!(prsp_status && strncmp(prsp_status, "0", 1) == 0))
	{
		std::string  ret_code = prsp_status ? prsp_status : "";
		std::string ret_msg = prsp_message ? prsp_message : "";

		CERROR_LOG("ret_code [%s] ret_msg [%s] \n", ret_code.c_str(), ret_msg.c_str());
		throw CException(ERR_CALL_UNIFIEDORDER_API_REQ, ret_msg.c_str(), __FILE__, __LINE__);
	}
	else
	{
		const char* prsp_result_code = GetXmlField(xmlElement, "result_code");
		if (!(prsp_result_code && strncmp(prsp_result_code, "0", 1) == 0))
		{
			const char* prsp_err_code = GetXmlField(xmlElement, "err_code");
			const char* prsp_err_msg = GetXmlField(xmlElement, "err_msg");
			CERROR_LOG("err_code : [%s] err_msg : [%s] \n", prsp_err_code, prsp_err_msg);
			throw CException(ERR_CALL_UNIFIEDORDER_API_REQ, prsp_err_msg, __FILE__, __LINE__);
		}
	}
	if (!CheckAgentSign(&wx_rsp_doc, inReq["mch_id"], inReq["pay_channel_id"]))
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_SIGN_CHECK, errMap[ERR_SIGN_CHECK].c_str());
		return ERR_SIGN_CHECK;
	}
	const char* szValue = NULL;
	int refund_count = (szValue = GetXmlField(xmlElement, "refund_count")) ? atoi(szValue) : 0;

	
	JsonList listJson;
	for (int i = 0; i < refund_count; ++i)
	{
		JsonMap columJson;
		std::string refund_id = GetXmlField(xmlElement, StringConcat("refund_id", i).c_str());
		std::string refund_no = GetXmlField(xmlElement, StringConcat("out_refund_no", i).c_str());
		std::string refund_fee = GetXmlField(xmlElement, StringConcat("refund_fee", i).c_str());
		std::string refund_time = GetXmlField(xmlElement, StringConcat("refund_time", i).c_str());
		std::string refund_status = GetXmlField(xmlElement, StringConcat("refund_status", i).c_str());	
		columJson.insert(JsonMap::value_type(JsonType("refund_id"), JsonType(refund_id)));
		columJson.insert(JsonMap::value_type(JsonType("refund_no"), JsonType(refund_no)));
		columJson.insert(JsonMap::value_type(JsonType("refund_fee"), JsonType(refund_fee)));
		columJson.insert(JsonMap::value_type(JsonType("refund_time"), JsonType(refund_time)));
		columJson.insert(JsonMap::value_type(JsonType("refund_status"), JsonType(refund_status)));
		
		listJson.push_back(columJson);
	}
	if (listJson.size())
	{
		outRsp.insert(std::make_pair("lists", inReq["listJson"]));
	}
	
	return 0;
}

int CSwiftPassPayApi::CloseOrder(StringMap& inReq, StringMap& outRsp)
{
	std::string szResBody = "";
	StringMap param;
	param.insert(StringMap::value_type("service", SWIFT_SERVICE_CLOSE_CMD));
	param.insert(StringMap::value_type("version", SWIFT_PASS_VERSION));
	param.insert(StringMap::value_type("mch_id", inReq["channel_mch_id"]));
	param.insert(StringMap::value_type("out_trade_no", inReq["order_no"]));
	param.insert(StringMap::value_type("nonce_str", toString(time(NULL))));

	SendMsgToApi(inReq["mch_id"], inReq["pay_channel_id"], base_url_, param, szResBody);

	tinyxml2::XMLDocument wx_rsp_doc;
	if (tinyxml2::XML_SUCCESS != wx_rsp_doc.Parse(szResBody.c_str(), szResBody.size()))
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_PASE_XML, errMap[ERR_PASE_XML].c_str());
		return ERR_PASE_XML;
	}

	tinyxml2::XMLElement * xmlElement = wx_rsp_doc.FirstChildElement("xml");
	if (NULL == xmlElement)
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_CREATE_XMLNode, errMap[ERR_CREATE_XMLNode].c_str());
		return ERR_CREATE_XMLNode;
	}

	const char* prsp_status = GetXmlField(xmlElement, "status");
	const char* prsp_message = GetXmlField(xmlElement, "message");
	if (!(prsp_status && strncmp(prsp_status, "0", 1) == 0))
	{
		std::string  ret_code = prsp_status ? prsp_status : "";
		std::string ret_msg = prsp_message ? prsp_message : "";

		CERROR_LOG("ret_code [%s] ret_msg [%s] \n", ret_code.c_str(), ret_msg.c_str());
		throw CException(ERR_CALL_UNIFIEDORDER_API_REQ, ret_msg.c_str(), __FILE__, __LINE__);
	}
	else
	{
		const char* prsp_result_code = GetXmlField(xmlElement, "result_code");
		if (!(prsp_result_code && strncmp(prsp_result_code, "0", 1) == 0))
		{
			const char* prsp_err_code = GetXmlField(xmlElement, "err_code");
			const char* prsp_err_msg = GetXmlField(xmlElement, "err_msg");
			CERROR_LOG("err_code : [%s] err_msg : [%s] \n", prsp_err_code, prsp_err_msg);
			throw CException(ERR_CALL_UNIFIEDORDER_API_REQ, prsp_err_msg, __FILE__, __LINE__);
		}
	}
	if (!CheckAgentSign(&wx_rsp_doc, inReq["mch_id"], inReq["pay_channel_id"]))
	{
		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_SIGN_CHECK, errMap[ERR_SIGN_CHECK].c_str());
		return ERR_SIGN_CHECK;
	}
	return 0;
}