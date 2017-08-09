/*
 * CRedisClient.cpp
 *
 *  Created on: 2012-12-12
 *      Author: sangechen
 */

#include "CSzSPDApi.h"
#include "log/clog.h"
#include "CCurlClient.h"
#include "url_protocol.h"
#include "CSocket.h"
#include "CRoutePayGateConfig.h"
#include "base64.h"
#include "url_protocol.h"
#include "error.h"

extern ErrParamMap errMap;

#define	 PIC_DIR		"../client/route_paygate/pic/"


#define  BASE_TEST_URL	"http://121.201.111.67:9080/payment-gate-web/"

#define  BASE_URL		"http://spdbweb.chinacardpos.com/payment-gate-web/"



#define  KEY_VERSION		"V1.1"

#define ADD_SHOP_TRANS_ID				"25"
#define REPORT_SHOP_TRANS_ID			"18"
#define REPORT_UPLOAD_PIC_TRANS_ID		"26"
#define QUERY_ORDER_TRANS_ID			"04"
#define REFUND_TRANS_ID					"02"
#define CLOSE_TRANS_ID					"03"

#define PAY_WAY_WX					"WX"
#define PAY_WAY_ALIPAY				"ALIPAY"

#define  BIG_PIC_TYP			1
#define  TRADE_TYPE_SIGN		2

CSzSPDApi::CSzSPDApi()
{
	base_url_	= BASE_URL;
	
	version_	= KEY_VERSION;

	base_beta_url_ = BASE_TEST_URL;
}

CSzSPDApi::~CSzSPDApi()
{
    
}

void CSzSPDApi::Reset()
{
	
}



int CSzSPDApi::AddShop(StringMap& inReq, StringMap& outRsp)
{
	BEGIN_LOG(__func__);

	std::string szResBody = "";
	/** 拼装 api AddShop 接口*/
	CDEBUG_LOG("Begin ...");

	std::string channel_mch_id("");
	if (inReq["channel_mch_id"].empty())
	{
		StringMap addShopParam;
		addShopParam.insert(std::make_pair("requestNo", GetRequestNo()));
		addShopParam.insert(std::make_pair("version", KEY_VERSION));
		addShopParam.insert(std::make_pair("transId", ADD_SHOP_TRANS_ID));
		addShopParam.insert(std::make_pair("agentId", inReq["agent_id"]));

		addShopParam.insert(std::make_pair("name", inReq["mch_name"]));
		addShopParam.insert(std::make_pair("nameAlias", inReq["mch_sname"]));
		addShopParam.insert(std::make_pair("mccValue", inReq["mccValue"]));
		addShopParam.insert(std::make_pair("legalPerson", inReq["legal_person"]));
		addShopParam.insert(std::make_pair("idcardNo", inReq["id_card_no"]));
		addShopParam.insert(std::make_pair("mobile", inReq["mobile"]));
		addShopParam.insert(std::make_pair("email", inReq["email"]));

		addShopParam.insert(std::make_pair("cityId", inReq["cityId"]));
		addShopParam.insert(std::make_pair("registerAddress", inReq["address"]));
		addShopParam.insert(std::make_pair("regNo", inReq["business_licence"]));

		addShopParam.insert(std::make_pair("regMoney", "300000000"));
		addShopParam.insert(std::make_pair("rateSchema", "219"));
		addShopParam.insert(std::make_pair("cardNoCipher", inReq["bank_cardno"]));
		addShopParam.insert(std::make_pair("cardName", inReq["bank_owner"]));
		addShopParam.insert(std::make_pair("cardBankNo", inReq["branch_no"]));
		addShopParam.insert(std::make_pair("isCompay", inReq["is_public"]));
		addShopParam.insert(std::make_pair("business", inReq["business"]));
		addShopParam.insert(std::make_pair("expireTime", "2999-01-01"));

		std::string  reqSignSrc = MakeSignSrc(addShopParam);

		CDEBUG_LOG("reqSignSrc=[%s]\n", reqSignSrc.c_str());

		std::string url = base_beta_url_ + "merchant/api/addMerchant";

		SendMsgToPay(url, reqSignSrc, inReq["agent_id"], szResBody);

		//进件接口返回是JSON String To Json
		JsonType jsonRes = JsonUtil::stringToObject(szResBody);
		JsonMap  jsonRspMap = jsonRes.toMap();

		if (0 != strcmp(GETJSONVALUE("respCode", jsonRspMap).c_str(), "0000")){
			CERROR_LOG("err_code:[%s] err_code_msg:[%s]\n", GETJSONVALUE("respCode", jsonRspMap).c_str(), GETJSONVALUE("respDesc", jsonRspMap).c_str());
			throw CException(ERR_CALL_ADD_SHOP_API_REQ, GETJSONVALUE("respDesc", jsonRspMap), __FILE__, __LINE__);
		}

		channel_mch_id = GETJSONVALUE("merNo", jsonRspMap);
		outRsp.insert(std::make_pair("channel_mch_id", channel_mch_id));

		if (!inReq["id_card_front_pic"].empty()) UploadPic(channel_mch_id, "idCard", inReq["agent_id"], inReq["id_card_front_pic"]);
		if (!inReq["id_card_pic"].empty()) UploadPic(channel_mch_id, "idCardFront", inReq["agent_id"], inReq["id_card_pic"]);
		if (!inReq["bank_card_pic"].empty()) UploadPic(channel_mch_id, "bankCardFile", inReq["agent_id"], inReq["bank_card_pic"]);
		if (!inReq["business_licence_pic"].empty()) UploadPic(channel_mch_id, "businessLicense", inReq["agent_id"], inReq["business_licence_pic"]);
	}
	else
	{
		channel_mch_id = inReq["channel_mch_id"];	
	}
	if (!channel_mch_id.empty())
	{
		int iRet = 0;
		//报备微信
		if (inReq["wx_sub_mch_id"].empty())
		{
			std::string wxSubMchId = "";
			ReportShop(channel_mch_id, PAY_WAY_WX, inReq, wxSubMchId);
			outRsp["wx_sub_mch_id"] = wxSubMchId;
		}
		
		//报备支付宝商户
		if (inReq["ali_sub_mch_id"].empty())
		{
			std::string aliSubMchId = "";
			ReportShop(channel_mch_id, PAY_WAY_ALIPAY, inReq, aliSubMchId);
			outRsp["ali_sub_mch_id"] = aliSubMchId;
		}
	}

	//报备微信商户
	

	return 0;
}

void CSzSPDApi::ReportShop(const std::string& channel_mch_id, const std::string& payWay, StringMap& inReq, std::string& payWaySubMchId)
{
	CDEBUG_LOG("Begin ...");
	std::string szResBody = "";

	StringMap reportShopParam;

	reportShopParam.insert(std::make_pair("requestNo", GetRequestNo()));
	reportShopParam.insert(std::make_pair("version", KEY_VERSION));
	reportShopParam.insert(std::make_pair("transId", REPORT_SHOP_TRANS_ID));
	reportShopParam.insert(std::make_pair("agentId", inReq["agent_id"]));

	reportShopParam.insert(std::make_pair("payWay", payWay));
	reportShopParam.insert(std::make_pair("merNo", channel_mch_id));	
	reportShopParam.insert(std::make_pair("subMechantName", inReq["mch_name"]));

	if (payWay == PAY_WAY_WX)
	{
		reportShopParam.insert(std::make_pair("business", inReq["wxpay_business"]));
	}
	else if (payWay == PAY_WAY_ALIPAY)
	{
		reportShopParam.insert(std::make_pair("business", inReq["alipay_business"]));

		//组装
		reportShopParam.insert(std::make_pair("addressInfo", inReq["addressInfo"]));
		reportShopParam.insert(std::make_pair("bankCardInfo", inReq["bankCardInfo"]));
		reportShopParam.insert(std::make_pair("contactInfo", inReq["contactInfo"]));
	}	
	reportShopParam.insert(std::make_pair("subMerchantShortname", inReq["mch_sname"]));

	reportShopParam.insert(std::make_pair("contact", inReq["head_man"]));
	reportShopParam.insert(std::make_pair("contactPhone", inReq["mobile"]));
	reportShopParam.insert(std::make_pair("contactEmail", inReq["email"]));

	reportShopParam.insert(std::make_pair("merchantRemark", channel_mch_id));
	reportShopParam.insert(std::make_pair("servicePhone", inReq["service_phone"]));

	std::string  reqSignSrc = MakeSignSrc(reportShopParam);

	CDEBUG_LOG("reqSignSrc=[%s]\n", reqSignSrc.c_str());

	std::string url = base_beta_url_ + "gateway/api/backTransReq";

	SendMsgToPay(url, reqSignSrc, inReq["agent_id"], szResBody);

	//报备微信或支付宝 返回是url形式
	StringMap urlRspMap;

	Kv2Map(szResBody, urlRspMap);

	if (0 != strcmp(GETSTRMAPVALUE("respCode", urlRspMap).c_str(), "0000")){
		CERROR_LOG("err_code:[%s] err_code_msg:[%s]\n", urlRspMap["respCode"].c_str(), urlRspMap["respDesc"].c_str());
		if (payWay == PAY_WAY_WX)
		{
			throw CException(ERR_CALL_WX_REPORT_SHOP_API_REQ, urlRspMap["respDesc"], __FILE__, __LINE__);
		}
		else if (payWay == PAY_WAY_ALIPAY)
		{
			throw CException(ERR_CALL_ALI_REPORT_SHOP_API_REQ, urlRspMap["respDesc"], __FILE__, __LINE__);
		}
		
	}
	payWaySubMchId = urlRspMap["subMchId"];

}

void CSzSPDApi::UploadPic(const std::string& channel_mch_id, 
				const std::string jsonKey, 
				const std::string& agentId, 
				const std::string& picUrl)
{
	BEGIN_LOG(__func__);

	std::string postUrl = base_beta_url_ + "upload/api/upFile";
	//下载图片
	CurlClient curl;

	curl.SetSSLVerify(false);

	CDEBUG_LOG("picUrl = [%s]", picUrl.c_str());

	int picIndex = picUrl.rfind('.');

	std::string picSuffix = picUrl.substr(picIndex + 1, picUrl.size() - picIndex - 1);

	CDEBUG_LOG("pirIndex = [%d] picSuffix = [%s]", picIndex, picSuffix.c_str());
	std::string  szFilePath = PIC_DIR + GetRequestNo() + "." + picSuffix;
	if (!curl.Save(picUrl, szFilePath.c_str()))
	{
		CERROR_LOG("curl save fail err_code=[%d] err_msg[%s], szResBody=[%s]",
			curl.GetErrCode(), curl.GetErrInfo().c_str(), szFilePath.c_str());
		throw CException(curl.GetErrCode(), curl.GetErrInfo(), __FILE__, __LINE__);
	}
	CDEBUG_LOG("szFilePath = [%s]\n", szFilePath.c_str());

	FILE* file = fopen(szFilePath.c_str(), "r");
	fseek(file, 0, SEEK_END);
	int file_size = ftell(file);
	rewind(file);
	unsigned char* pBmpBuf = new unsigned char[file_size + 1];
	fread(pBmpBuf, sizeof(char), file_size + 1, file);
	fclose(file);
	CDEBUG_LOG("file_size = [%d]", file_size);

	std::string picDetailStr("");
	picDetailStr.reserve(file_size + 1);
	picDetailStr = base64Encode(pBmpBuf, file_size);
	CDEBUG_LOG("base64Encode len:[%d] baseCode:[%s]", picDetailStr.length(), picDetailStr.c_str());

	delete []pBmpBuf;

	JsonMap picJson;
	picJson.insert((JsonMap::value_type(JsonType("base64Code"), JsonType(picDetailStr))));
	picJson.insert((JsonMap::value_type(JsonType("fileType"), JsonType(picSuffix))));

	JsonMap purPoseMap;
	purPoseMap.insert(JsonMap::value_type(JsonType(jsonKey), JsonType(picJson)));


	StringMap uploadMap;
	uploadMap.insert(std::make_pair("requestNo", GetRequestNo()));
	uploadMap.insert(std::make_pair("version", KEY_VERSION));
	uploadMap.insert(std::make_pair("transId", REPORT_UPLOAD_PIC_TRANS_ID));
	uploadMap.insert(std::make_pair("agentId", agentId));
	uploadMap.insert(std::make_pair("spId", channel_mch_id));
	uploadMap.insert(std::make_pair("reportType", "SH"));
	uploadMap.insert(std::make_pair("upFiles", JsonUtil::objectToString(purPoseMap)));

	std::string uploadRspStr("");

	std::string  reqSignSrc = MakeSignSrc(uploadMap);
	//保存文件
	std::string wfileName = "../client/route_paygate/lib/" + GetRequestNo() + ".txt";
	FILE* wfile = fopen(wfileName.c_str(), "wb");
	fwrite(reqSignSrc.c_str(), reqSignSrc.size(), 1, wfile);
	fclose(wfile);

	SendMsgToPay(postUrl, reqSignSrc, agentId, uploadRspStr, BIG_PIC_TYP);

	JsonType jsonRes = JsonUtil::stringToObject(uploadRspStr);
	JsonMap  jsonRspMap = jsonRes.toMap();

	if (0 != strcmp(GETJSONVALUE("respCode", jsonRspMap).c_str(), "0000")){
		CERROR_LOG("picUrl:[%s] err_code:[%s] err_code_msg:[%s]\n", picUrl.c_str(), GETJSONVALUE("respCode", jsonRspMap).c_str(), GETJSONVALUE("respDesc", jsonRspMap).c_str());
		//throw CException(ERR_CHANNEL_AGENT_ID_PARAM_INVALID, GETJSONVALUE("respDesc", jsonRspMap), __FILE__, __LINE__);
	}
}


//idStr 进件传agentId  支付传 mchId 
void CSzSPDApi::SendMsgToPay(const std::string& url, const std::string& reqSignSrc, const std::string& idStr, std::string& szResBody, int type /* = 0 */)
{
	BEGIN_LOG(__func__);
	CurlClient curl;

	std::string reqSign = "", reqMsg = "", szResHead = "";

	curl.SetSSLVerify(false);

	curl.SetTimeOut(20);

	if (type == BIG_PIC_TYP)
	{
		//reqSign = CreatePicSign(idStr, reqSignSrc);
		reqSign = CreateAgentSign(idStr, reqSignSrc, BIG_PIC_TYP);
	}
	else if (type == TRADE_TYPE_SIGN)
	{
		//交易验检
		reqSign = CreateAgentSign(idStr, reqSignSrc, TRADE_TYPE_SIGN);
	}
	else
	{
		reqSign = CreateAgentSign(idStr, reqSignSrc);
	}
	CDEBUG_LOG("sign = [%s]\n", reqSign.c_str());

	reqMsg = reqSignSrc + "&signature=" + reqSign;

	CDEBUG_LOG("szReqUrl=[%s], reqMsg=[%s]\n", url.c_str(), reqMsg.c_str());
	gettimeofday(&stStart, NULL);
	if (!curl.Post(url, reqMsg, szResHead, szResBody))
	{
		CERROR_LOG("curl post fail err_code=[%d] err_msg[%s] szResHead=[%s],szResBody=[%s]", 
			curl.GetErrCode(), curl.GetErrInfo().c_str(), szResHead.c_str(), szResBody.c_str());
		throw CException(curl.GetErrCode(), curl.GetErrInfo(), __FILE__, __LINE__);
	}
	gettimeofday(&stEnd, NULL);
	int iUsedTime = CalDiff(stStart, stEnd);
	CDEBUG_LOG("szResHead=[%s], szResBody=[%s] iUsedTime[%d] \n", szResHead.c_str(), szResBody.c_str(), iUsedTime);
}


int CSzSPDApi::Unifiedorder(StringMap& inReq, StringMap& outRsp)
{
	BEGIN_LOG(__func__);
	CDEBUG_LOG("Begin ...");
	std::string szResBody = "";
	StringMap placeOrderMap;
	placeOrderMap.insert(std::make_pair("requestNo", inReq["order_no"]));
	placeOrderMap.insert(std::make_pair("version", KEY_VERSION));

	std::vector<string> vecPayService = split(inReq["pay_service"], ",");
	if (vecPayService.size() != 2)
	{
		throw CException(ERR_PRODUCT_PAY_SERVICE, errMap[ERR_PRODUCT_PAY_SERVICE], __FILE__, __LINE__);
	}
	placeOrderMap.insert(std::make_pair("productId", vecPayService[0]));
	placeOrderMap.insert(std::make_pair("transId", vecPayService[1]));

	placeOrderMap.insert(std::make_pair("clientIp", inReq["spbill_create_ip"]));
	//placeOrderMap.insert(std::make_pair("agentId", inReq["channel_factor_id"]));
	placeOrderMap.insert(std::make_pair("merNo", inReq["channel_mch_id"]));
	placeOrderMap.insert(std::make_pair("subMchId", inReq["channel_sub_mch_id"]));

	placeOrderMap.insert(std::make_pair("orderDate", getSysDate()));
	placeOrderMap.insert(std::make_pair("orderNo", inReq["order_no"]));
	if (inReq["pay_type"] == TRADE_TYPE_MICROPAY)
	{
		placeOrderMap.insert(std::make_pair("autoCode", inReq["auth_code"]));
	}
	else if (inReq["pay_type"] == TRADE_TYPE_JSAPI)
	{
		placeOrderMap.insert(std::make_pair("subOpenId", inReq["open_id"]));
	}
	placeOrderMap.insert(std::make_pair("returnUrl", inReq["return_url"]));
	placeOrderMap.insert(std::make_pair("notifyUrl", inReq["notify_url"]));

	placeOrderMap.insert(std::make_pair("transAmt", inReq["total_fee"]));
	placeOrderMap.insert(std::make_pair("commodityName", inReq["body"]));

	if (!inReq["limit_credit_pay"].empty()) placeOrderMap.insert(std::make_pair("limitPay", inReq["limit_credit_pay"]));
	if (!inReq["time_expire"].empty()) placeOrderMap.insert(std::make_pair("timeExpire", inReq["time_expire"]));

	std::string  reqSignSrc = MakeSignSrc(placeOrderMap);

	CDEBUG_LOG("reqSignSrc=[%s]\n", reqSignSrc.c_str());

	SendMsgToPay(inReq["api_url"], reqSignSrc, inReq["mch_id"], szResBody, TRADE_TYPE_SIGN);
	StringMap urlRspMap;

	Kv2Map(szResBody, urlRspMap);

	if (0 != strcmp(urlRspMap["respCode"].c_str(), "0000")){
		CERROR_LOG("err_code:[%s] err_code_msg:[%s]\n", urlRspMap["respCode"].c_str(), urlRspMap["respDesc"].c_str());
		if (0 == strcmp(urlRspMap["respCode"].c_str(), "P000"))
		{
			return ERR_TRADE_PROCESS_ING;
		}
		throw CException(ERR_CALL_UNIFIEDORDER_API_REQ, urlRspMap["respDesc"], __FILE__, __LINE__);
	}

	if (inReq["pay_type"] == TRADE_TYPE_NATIVE)
	{
		std::string codeUrl = base64_decode(GETSTRMAPVALUE("codeUrl", urlRspMap));
		CDEBUG_LOG("codeUrl : [%s]", codeUrl.c_str());
		outRsp.insert(std::make_pair("code_url",codeUrl));
	}
	else if (inReq["pay_type"] == TRADE_TYPE_JSAPI)
	{
		std::string prepay_id = base64_decode(urlRspMap["payInfo"]);
		outRsp.insert(std::make_pair("prepay_id", prepay_id));
	}
	outRsp.insert(std::make_pair("order_no", inReq["order_no"]));
	outRsp.insert(std::make_pair("total_fee", inReq["total_fee"]));
	return 0;
}

int CSzSPDApi::Micropay(StringMap& inReq, StringMap& outRsp)
{
	
	return 0;
}

int CSzSPDApi::OrderQuery(StringMap& inReq, StringMap& outRsp)
{
	std::string szResBody = "";
	StringMap paramMap;
	paramMap.insert(std::make_pair("requestNo", GetRequestNo()));
	paramMap.insert(std::make_pair("version", KEY_VERSION));
	paramMap.insert(std::make_pair("transId", QUERY_ORDER_TRANS_ID));
	paramMap.insert(std::make_pair("merNo", inReq["channel_mch_id"]));
	paramMap.insert(std::make_pair("orderNo", inReq["order_no"]));

	std::string orderDate = inReq["order_no"].substr(3, 8);

	paramMap.insert(std::make_pair("orderDate", orderDate));

	std::string  reqSignSrc = MakeSignSrc(paramMap);

	CDEBUG_LOG("reqSignSrc=[%s]\n", reqSignSrc.c_str());

	std::string apiUrl = base_url_ + "gateway/api/backTransReq";

	SendMsgToPay(apiUrl, reqSignSrc, inReq["mch_id"], szResBody, TRADE_TYPE_SIGN);

	StringMap urlRspMap;

	Kv2Map(szResBody, urlRspMap);

	if (0 != strcmp(urlRspMap["respCode"].c_str(), "0000")){
		CERROR_LOG("err_code:[%s] err_code_msg:[%s]\n", urlRspMap["respCode"].c_str(), urlRspMap["respDesc"].c_str());
		throw CException(ERR_CALL_QUERY_API_REQ, urlRspMap["respDesc"], __FILE__, __LINE__);
	}
	std::string order_status("");
	if (0 == strcmp(urlRspMap["origRespCode"].c_str(), "0000"))
	{
		order_status = "SUCCESS";
	}
	else
	{
		order_status = "NOTPAY";
	}
	outRsp.insert(std::make_pair("order_no", inReq["order_no"]));
	outRsp.insert(std::make_pair("order_status", order_status));
	outRsp.insert(std::make_pair("total_fee", urlRspMap["transAmt"]));
	outRsp.insert(std::make_pair("refund_fee", urlRspMap["refundAmt"]));

	return 0;
}

int CSzSPDApi::Refund(StringMap& inReq, StringMap& outRsp)
{
	BEGIN_LOG(__func__);
	std::string szResBody = "";
	StringMap paramMap;
	paramMap.insert(std::make_pair("requestNo", GetRequestNo()));
	paramMap.insert(std::make_pair("version", KEY_VERSION));
	paramMap.insert(std::make_pair("transId", REFUND_TRANS_ID));

	paramMap.insert(std::make_pair("merNo", inReq["channel_mch_id"]));

	paramMap.insert(std::make_pair("orderDate", getSysDate()));
	paramMap.insert(std::make_pair("orderNo", inReq["refund_no"]));

	std::string orderDate = inReq["order_no"].substr(3, 8);
	paramMap.insert(std::make_pair("origOrderDate", orderDate));
	paramMap.insert(std::make_pair("origOrderNo", inReq["order_no"]));

	paramMap.insert(std::make_pair("notifyUrl", inReq["notify_url"]));
	paramMap.insert(std::make_pair("returnUrl", inReq["return_url"]));

	paramMap.insert(std::make_pair("transAmt", inReq["refund_fee"]));
	paramMap.insert(std::make_pair("refundReson", inReq["refund_cause"]));

	std::string  reqSignSrc = MakeSignSrc(paramMap);

	CDEBUG_LOG("reqSignSrc=[%s]\n", reqSignSrc.c_str());

	std::string apiUrl = base_url_ + "gateway/api/backTransReq";

	SendMsgToPay(apiUrl, reqSignSrc, inReq["mch_id"], szResBody, TRADE_TYPE_SIGN);

	StringMap urlRspMap;

	Kv2Map(szResBody, urlRspMap);

	if (0 != strcmp(urlRspMap["respCode"].c_str(), "0000")){
		CERROR_LOG("err_code:[%s] err_code_msg:[%s]\n", urlRspMap["respCode"].c_str(), urlRspMap["respDesc"].c_str());
		throw CException(ERR_CALL_REFUND_API_REQ, urlRspMap["respDesc"], __FILE__, __LINE__);
	}

	return 0;
}

int CSzSPDApi::RefundQuery(StringMap& inReq, StringMap& outRsp)
{
	BEGIN_LOG(__func__);
	std::string szResBody = "";
	StringMap paramMap;
	paramMap.insert(std::make_pair("requestNo", GetRequestNo()));
	paramMap.insert(std::make_pair("version", KEY_VERSION));
	paramMap.insert(std::make_pair("transId", QUERY_ORDER_TRANS_ID));
	paramMap.insert(std::make_pair("merNo", inReq["channel_mch_id"]));
	paramMap.insert(std::make_pair("orderNo", inReq["refund_no"]));

	std::string orderDate = inReq["refund_no"].substr(1, 8);

	paramMap.insert(std::make_pair("orderDate", orderDate));

	std::string  reqSignSrc = MakeSignSrc(paramMap);

	CDEBUG_LOG("reqSignSrc=[%s]\n", reqSignSrc.c_str());

	std::string apiUrl = base_url_ + "gateway/api/backTransReq";

	SendMsgToPay(apiUrl, reqSignSrc, inReq["mch_id"], szResBody, TRADE_TYPE_SIGN);

	StringMap urlRspMap;

	Kv2Map(szResBody, urlRspMap);

	if (0 != strcmp(urlRspMap["respCode"].c_str(), "0000")){
		CERROR_LOG("err_code:[%s] err_code_msg:[%s]\n", urlRspMap["respCode"].c_str(), urlRspMap["respDesc"].c_str());
		throw CException(ERR_CALL_QUERY_API_REQ, urlRspMap["respDesc"], __FILE__, __LINE__);
	}
	std::string order_status("");
	if (0 == strcmp(urlRspMap["origRespCode"].c_str(), "0000"))
	{
		order_status = "REFUND";
	}
	else
	{
		order_status = "FAIL";
	}
	outRsp.insert(std::make_pair("refund_no", inReq["refund_no"]));
	outRsp.insert(std::make_pair("refund_status", order_status));
	outRsp.insert(std::make_pair("total_fee", urlRspMap["transAmt"]));
	outRsp.insert(std::make_pair("refund_fee", urlRspMap["refundAmt"]));

	return 0;
}

int CSzSPDApi::CloseOrder(StringMap& inReq, StringMap& outRsp)
{
	BEGIN_LOG(__func__);

	std::string szResBody = "";
	StringMap paramMap;
	paramMap.insert(std::make_pair("requestNo", GetRequestNo()));
	paramMap.insert(std::make_pair("version", KEY_VERSION));
	paramMap.insert(std::make_pair("transId", CLOSE_TRANS_ID));
	paramMap.insert(std::make_pair("merNo", inReq["channel_mch_id"]));

	paramMap.insert(std::make_pair("orderNo", GetRequestNo()));
	paramMap.insert(std::make_pair("orderDate", getSysDate()));
	
	paramMap.insert(std::make_pair("origOrderNo", inReq["order_no"]));

	paramMap.insert(std::make_pair("notifyUrl", inReq["notify_url"]));
	paramMap.insert(std::make_pair("returnUrl", inReq["return_url"]));

	paramMap.insert(std::make_pair("transAmt", inReq["total_fee"]));
	
	std::string  reqSignSrc = MakeSignSrc(paramMap);

	CDEBUG_LOG("reqSignSrc=[%s]\n", reqSignSrc.c_str());

	std::string apiUrl = base_url_ + "gateway/api/backTransReq";

	SendMsgToPay(apiUrl, reqSignSrc, inReq["mch_id"], szResBody, TRADE_TYPE_SIGN);

	StringMap urlRspMap;

	Kv2Map(szResBody, urlRspMap);

	if (0 != strcmp(urlRspMap["respCode"].c_str(), "0000")){
		CERROR_LOG("err_code:[%s] err_code_msg:[%s]\n", urlRspMap["respCode"].c_str(), urlRspMap["respDesc"].c_str());
		throw CException(ERR_CALL_REFUND_API_REQ, urlRspMap["respDesc"], __FILE__, __LINE__);
	}

	return 0;
}


std::string CSzSPDApi::GetRequestNo()
{
	return GetSysTimeUsecEx(time(NULL));
}

string CSzSPDApi::CreateAgentSign(const string& idStr, const string& plainText, int type /* = 0 */)
{
	//return SHA256RSASign(plainText, priKey);
	int iRet = 0;

	std::string szSignStr("");

	NameValueMap AuthReq;
	AuthReq.insert(std::make_pair("ver", "1.0"));
	//调用签名接口
	AuthReq.insert(std::make_pair("cmd", "1004"));

	JsonMap  bizCJsonMap;
	bizCJsonMap.insert(JsonMap::value_type(JsonType("pay_channel_id"), JsonType(SZ_SPD_PAI_PAY_CHANNEL)));
	if (!type)
	{
		bizCJsonMap.insert(JsonMap::value_type(JsonType("type"), JsonType("channel")));
		bizCJsonMap.insert(JsonMap::value_type(JsonType("channel_factor_id"), JsonType(idStr)));
	}
	else if (type == 2)
	{
		bizCJsonMap.insert(JsonMap::value_type(JsonType("type"), JsonType("channel_mch")));
		bizCJsonMap.insert(JsonMap::value_type(JsonType("mch_id"), JsonType(idStr)));
	}	
	bizCJsonMap.insert(JsonMap::value_type(JsonType("src_str"), JsonType(plainText)));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("digest_type"), JsonType("SHA1")));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("rsa_func_type"), JsonType("2")));
	bizCJsonMap.insert(JsonMap::value_type(JsonType("pay_type"), JsonType("upay")));
	
	std::string bizContent = JsonUtil::objectToString(bizCJsonMap);

	AuthReq.insert(std::make_pair("biz_content", bizContent));

	CSocket *pSocket = Singleton<CRoutePayGateConfig>::GetInstance()->GetAuthSocket();

	if (type == BIG_PIC_TYP)
	{
		char szRsp[1024 * 1024] = { 0 };
		char szSign[1024 * 1024] = { 0 };

		iRet = pSocket->SendAndRecvLineStr(AuthReq, szRsp, 1024 * 10, "\r\n");
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

		clib_urlencode_comm_ex(rspSign.c_str(), szSign, 1024 * 1024);
	}
	else
	{
		char szRsp[1024 * 10] = { 0 };
		char szSign[1024 * 10] = { 0 };

		iRet = pSocket->SendAndRecvLineEx(AuthReq, szRsp, 1024 * 10, "\r\n");
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
	}
	

	return szSignStr;
}

string CSzSPDApi::CreatePicSign(const string& agentId, const string& plainText)
{
	std::string key = "../client/route_paygate/conf/SZSPDPAY_100003_rsa_pri.pem";
	return RSASign(plainText, key, "SHA1", "2");
}

std::string CSzSPDApi::MakeSignSrc(StringMap& paramMap)
{
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
	return content;
}
