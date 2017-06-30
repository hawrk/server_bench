/*
 * OrderServerV2.cpp
 *
 *  Created on: Nov 14, 2011
 *      Author: sangechen
 */

#include "CSpeedPosServer.h"
#include "utils.h"
#include "TenMchpayClient.h"
#include "CSpeedPosConfig.h"
//#include "ThreadPool_wrapper.h"
//#include "CSingleLimitAtLeast.hpp"
#include "../../Base/Comm/UserInfoClient.h"
#include "libssh2.h"
#include "libssh2_sftp.h"
#include "libssh2_publickey.h"
#include "bill_protocol.h"
#include "http_client.h"
#include "log/clog.h"
#include "common.h"
#include "cJSON.h"
#include "json_util.h"
#include "urlparammap.h"
#include "network.h"
#include "util/tc_file.h"


CSpeedPosServer::CSpeedPosServer()
{
	
}

CSpeedPosServer::~CSpeedPosServer()
{
	
}

INT32 CSpeedPosServer::Init(const CommServer& stTradeGenServer)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	iRet = sppClent.Init(stTradeGenServer);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "Clent Init Failed.Ret[%d] Err[%s]",
			iRet, sppClent.GetErrorMessage());
		return ORDER_RET_SYSERR;
	}
	return 0;
}

static size_t write_data(void *ptr, size_t sSize, size_t sNmemb, void *stream)
{
	std::string strBuf = std::string(static_cast<char *>(ptr), sSize * sNmemb);
	std::stringstream *ssResponse = static_cast<std::stringstream *>(stream);
	*ssResponse << strBuf;
	return sSize * sNmemb;
}


size_t DownloadCallback(void* pBuffer, size_t nSize, size_t nMemByte, void* pParam)
{
	FILE* fp = (FILE*)pParam;
	size_t nWrite = fwrite(pBuffer, nSize, nMemByte, fp);

	return nWrite;
}

int ProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	//CSpeedPosServer* dd = (CSpeedPosServer*)clientp;

	if (dltotal > -0.1 && dltotal < 0.1)
	{
		return 0;
	}
	int nPos = (int)((dlnow / dltotal) * 100);
	//通知进度条更新下载进度  
	CDEBUG_LOG("dltotal: %d  ---- dlnow: %d ,pos = [%d]\n", (long)dltotal, (long)dlnow,nPos);

	return 0;
}
/**
* brief:
* param:
* out:
* return: succ:RET_SUCCESS_UNIX
* */
int CSpeedPosServer::SendRequestWxapi(const std::string& sign_key, 
									const std::string& pay_ali_url, 
									StringMap& paramMap, 
									std::string& responseStr)
{
	BEGIN_LOG(__func__);
	//INT32 iRet = 0;
	CURL *pCurl = NULL;
	CDEBUG_LOG("send_request_wxapi begin \n");
	tinyxml2::XMLPrinter oPrinter;
	StringMap::const_iterator iter;
	char szSign[33];
	tinyxml2::XMLDocument doc_wx;
	tinyxml2::XMLNode* pXmlNode = doc_wx.InsertEndChild(doc_wx.NewElement("xml"));
	if (!pXmlNode)
	{
		return ERR_CREATE_XMLNode;
	}
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
	char szBuf[1024] = { 0 };
	snprintf(szBuf, sizeof(szBuf), "%s&key=%s", content.c_str(), sign_key.c_str());
	//CDEBUG_LOG("content : [%s]\n", szBuf);
	GetMd5(szBuf, strlen(szBuf), szSign);
	for (iter = paramMap.begin();
		iter != paramMap.end(); ++iter)
	{
		SetOneFieldToXml(&doc_wx, pXmlNode, iter->first.c_str(), iter->second.c_str(), false);
	}
	if (SetOneFieldToXml(&doc_wx, pXmlNode, "sign", szSign, false) != 0);
	doc_wx.Accept(&oPrinter);
	std::string strReq = oPrinter.CStr();
	pCurl = curl_easy_init();
	CDEBUG_LOG("send Request: [%s]\n", strReq.c_str());
	std::stringstream ssBody;
	//curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 2);
	curl_easy_setopt(pCurl, CURLOPT_URL, pay_ali_url.c_str());
	curl_easy_setopt(pCurl, CURLOPT_HEADER, false);
	curl_easy_setopt(pCurl, CURLOPT_POST, true);
	curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, strReq.c_str());
	curl_easy_setopt(pCurl, CURLOPT_POSTFIELDSIZE, strReq.size());
	curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &ssBody);

	CURLcode eRetCode = curl_easy_perform(pCurl);
	if (CURLE_OK != eRetCode)
	{
		return eRetCode;
	}
	curl_easy_cleanup(pCurl);
	pCurl = NULL;
	const std::string& strBody = ssBody.str();
	//CDEBUG_LOG("recv : [%s]\n", strBody.c_str());
	responseStr = strBody;
	return 0;
}

int CSpeedPosServer::SendRequestDataApi(const std::string& sign_key, 
							const std::string& pay_ali_url, StringMap& paramMap, 
							std::string& responseStr)
{
	BEGIN_LOG(__func__);
	//INT32 iRet = 0;
	CURL *pCurl = NULL;
	char szSign[33];
	CDEBUG_LOG("SendRequestDataApi begin \n");
	std::string content = "";
	StringMap::const_iterator iter;
	for (iter = paramMap.begin();
		iter != paramMap.end(); ++iter) {
		if (!content.empty()) {
			content.push_back('&');
		}
		content.append(iter->first);
		content.push_back('=');
		content.append(iter->second);
	}
	char szBuf[1024] = { 0 };
	snprintf(szBuf, sizeof(szBuf), "%s&key=%s", content.c_str(), sign_key.c_str());
	//CDEBUG_LOG("content : [%s]\n", szBuf);
	GetMd5(szBuf, strlen(szBuf), szSign);
	std::string strReq = content + "&sign=" + szSign;
	pCurl = curl_easy_init();
	CDEBUG_LOG("send : [%s]\n", strReq.c_str());
	std::stringstream ssBody;
	//curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 2);
	curl_easy_setopt(pCurl, CURLOPT_URL, pay_ali_url.c_str());
	curl_easy_setopt(pCurl, CURLOPT_HEADER, false);
	curl_easy_setopt(pCurl, CURLOPT_POST, true);
	curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, strReq.c_str());
	curl_easy_setopt(pCurl, CURLOPT_POSTFIELDSIZE, strReq.size());
	curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &ssBody);

	CURLcode eRetCode = curl_easy_perform(pCurl);
	if (CURLE_OK != eRetCode)
	{
		return eRetCode;
	}
	curl_easy_cleanup(pCurl);
	pCurl = NULL;
	const std::string& strBody = ssBody.str();
	if (strBody.empty())
	{
		return -10;
	}
	responseStr = strBody;
	return 0;
}

int CSpeedPosServer::SendRequestDownload(const std::string strUrl, std::string& path)
{
	BEGIN_LOG(__func__);
	//INT32 iRet = 0;
	CURL *pCurl = NULL;
	//char szSign[33];
	CDEBUG_LOG("SendRequestDownload begin \n");
	//FILE *fp = fopen(outfilename.c_str(), "wb");
	pCurl = curl_easy_init();
	std::stringstream ssBody;
	//curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 2);
	FILE* file = fopen(path.c_str(), "wb");
	curl_easy_setopt(pCurl, CURLOPT_URL, strUrl.c_str());
	curl_easy_setopt(pCurl, CURLOPT_HEADER, false);
	curl_easy_setopt(pCurl, CURLOPT_ENCODING, L"zip,deflate");
	curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, DownloadCallback);
	curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, file);
	curl_easy_setopt(pCurl, CURLOPT_PROGRESSFUNCTION, ProgressCallback);
	curl_easy_setopt(pCurl, CURLOPT_PROGRESSDATA, this);

	CURLcode eRetCode = curl_easy_perform(pCurl);
	if (CURLE_OK != eRetCode)
	{
		fclose(file);
		const char* pError = curl_easy_strerror(eRetCode);
		CERROR_LOG("retCode : %d error %s \n", eRetCode, pError);
		return eRetCode;
	}	
	curl_easy_cleanup(pCurl);
	pCurl = NULL;
	fclose(file);
	/*const std::string& strBody = ssBody.str();
	if (strBody.empty())
	{
		return -10;
	}
	responseStr = strBody;*/
	return 0;
}

int CSpeedPosServer::SendRequestTradeServapi(const std::string& sign_key,
									const std::string& trade_serv_url,
									StringMap& paramMap,
									std::string& responseStr)
{
	BEGIN_LOG(__func__);
	//INT32 iRet = 0;
	CURL *pCurl = NULL;
	CDEBUG_LOG("send_request_trade_server begin :url = [%s]",trade_serv_url.c_str());
	tinyxml2::XMLPrinter oPrinter;
	StringMap::const_iterator iter;
	char szSign[33];
	tinyxml2::XMLDocument doc_wx;
	tinyxml2::XMLNode* pXmlNode = doc_wx.InsertEndChild(doc_wx.NewElement("xml"));
	if (!pXmlNode)
	{
		return ERR_CREATE_XMLNode;
	}
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
	char szBuf[1024] = { 0 };
	snprintf(szBuf, sizeof(szBuf), "%s&key=%s", content.c_str(), sign_key.c_str());
	//CDEBUG_LOG("content : [%s]\n", szBuf);
	GetMd5(szBuf, strlen(szBuf), szSign);
	for (iter = paramMap.begin();
		iter != paramMap.end(); ++iter)
	{
		SetOneFieldToXml(&doc_wx, pXmlNode, iter->first.c_str(), iter->second.c_str(), false);
	}
	if (SetOneFieldToXml(&doc_wx, pXmlNode, "sign", szSign, false) != 0);
	doc_wx.Accept(&oPrinter);
	std::string strReq = oPrinter.CStr();
	pCurl = curl_easy_init();
	CDEBUG_LOG("send trade server Request: [%s]\n", strReq.c_str());
	std::stringstream ssBody;
	//curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 2);
	curl_easy_setopt(pCurl, CURLOPT_URL, trade_serv_url.c_str());
	curl_easy_setopt(pCurl, CURLOPT_HEADER, false);
	curl_easy_setopt(pCurl, CURLOPT_POST, true);
	curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, strReq.c_str());
	curl_easy_setopt(pCurl, CURLOPT_POSTFIELDSIZE, strReq.size());
	curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &ssBody);

	CURLcode eRetCode = curl_easy_perform(pCurl);
	if (CURLE_OK != eRetCode)
	{
		return eRetCode;
	}
	curl_easy_cleanup(pCurl);
	pCurl = NULL;
	const std::string& strBody = ssBody.str();
	CDEBUG_LOG("recv : [%s]\n", strBody.c_str());
	responseStr = strBody;
	return 0;
}

/**
* brief:
* param:
* out:
* return: succ:RET_SUCCESS_UNIX
* */
int  CSpeedPosServer::SetOneFieldToXml(tinyxml2::XMLDocument * pDoc, tinyxml2::XMLNode* pXmlNode, const char * pcFieldName,
	const char* pszValue, bool bIsCdata)
{
	if (!pszValue || strlen(pszValue) == 0)
	{
		return 0;
	}

	if (!pDoc || !pXmlNode || !pcFieldName)
	{
		return -1;
	}

	tinyxml2::XMLElement * pFiledElement = pDoc->NewElement(pcFieldName);
	if (NULL == pFiledElement)
	{
		return -1;
	}

	tinyxml2::XMLText * pText = pDoc->NewText(pszValue);
	if (NULL == pText)
	{
		return -1;
	}

	pText->SetCData(bIsCdata);
	pFiledElement->LinkEndChild(pText);

	pXmlNode->LinkEndChild(pFiledElement);
	return 0;
}

/**
* brief:
* param:
* out:
* return: succ:RET_SUCCESS_UNIX
* */
int CSpeedPosServer::CallGetBankNoApi(TRemitBill& remitBill)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	time_t tNow = time(NULL);
	//load config
	CBillBusiConfig* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;

	StringMap getBankNoMap;
	getBankNoMap.insert(StringMap::value_type("_nonce_str", toString(tNow)));
	getBankNoMap.insert(StringMap::value_type("_timestamp", toString(tNow)));
	getBankNoMap.insert(StringMap::value_type("_version", "1.0"));
	getBankNoMap.insert(StringMap::value_type("accountid", remitBill.account_id));
	getBankNoMap.insert(StringMap::value_type("type", remitBill.sType));
	getBankNoMap.insert(StringMap::value_type("_datatype", "single_json"));
	std::string strGetBankNoRsp;
	iRet = SendRequestDataApi(mainConfig.sApiKey, mainConfig.sGetBankNoApiUrl, getBankNoMap, strGetBankNoRsp);
	CDEBUG_LOG(" GetBankNoApiUrl url:[%s] strGetBankNoRsp:%s\n", mainConfig.sGetBankNoApiUrl.c_str(), strGetBankNoRsp.c_str());

	cJSON* root = cJSON_Parse(strGetBankNoRsp.c_str());
	cJSON* error  = cJSON_GetObjectItem(root, "error");
	iRet = error->valueint;
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "SendRequestDataApi failed ! "
			"Ret[%d].",
			iRet);
		CERROR_LOG("SendRequestDataApi failed! "
			"Ret[%d].\n",
			iRet);
		return -1010;
	}
	JsonType jsonObj = JsonUtil::stringToObject(strGetBankNoRsp);
	JsonMap rspMap = jsonObj.toMap();
	JsonMap::const_iterator iterJson;
	if ((iterJson = rspMap.find("bank_cardno")) != rspMap.end()) remitBill.sBankCardNo = iterJson->second.toString();
	if ((iterJson = rspMap.find("bank_owner")) != rspMap.end()) remitBill.sBankOwner = iterJson->second.toString();
	if ((iterJson = rspMap.find("account_name")) != rspMap.end()) remitBill.sName = iterJson->second.toString();
	if ((iterJson = rspMap.find("is_public")) != rspMap.end()) remitBill.sBankCardType = iterJson->second.toString();  //银行卡号对公对私
	if ((iterJson = rspMap.find("bank_type")) != rspMap.end()) remitBill.sBankType = iterJson->second.toString();  //银行类型
	if ((iterJson = rspMap.find("branch_no")) != rspMap.end()) remitBill.sBranchNo = iterJson->second.toString();   //银行网点号
	if ((iterJson = rspMap.find("shop_name")) != rspMap.end()) remitBill.sShopName = iterJson->second.toString();  //商户名称
	if ((iterJson = rspMap.find("cycle")) != rspMap.end()) remitBill.sCycle = iterJson->second.toString();    //结算周期

	CDEBUG_LOG(" sBankCardNo:[%s] sBankCardType[%s]:sBankOwner:[%s] sShopName[%s]\n",
			remitBill.sBankCardNo.c_str(), remitBill.sBankCardType.c_str(),remitBill.sBankOwner.c_str(), remitBill.sName.c_str());
	return 0;
}

/**
* brief:
* param:
* out:
* return: succ:RET_SUCCESS_UNIX
* */
int CSpeedPosServer::CallGetPayFailApi(const std::string& strBmId, int& iIndex, const std::string& sPayChannel,
				const std::string& strPayTime, std::map<int, TRemitBill>& tremitMap)
{
	BEGIN_LOG(__func__);
	//INT32 iRet = 0;
	time_t tNow = time(NULL);
	//load config
	CBillBusiConfig* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;
	StringMap getPayFailMap;
	getPayFailMap.insert(StringMap::value_type("_nonce_str", toString(tNow)));
	getPayFailMap.insert(StringMap::value_type("_timestamp", toString(tNow)));
	getPayFailMap.insert(StringMap::value_type("_version", "1.0"));
	getPayFailMap.insert(StringMap::value_type("dateflag", toDate(strPayTime)));
	getPayFailMap.insert(StringMap::value_type("bm_id", strBmId));
	getPayFailMap.insert(StringMap::value_type("pay_channel", sPayChannel));

	std::string strGetPayFailRsp;
	SendRequestDataApi(mainConfig.sApiKey, mainConfig.sGetPayFailUrl, getPayFailMap, strGetPayFailRsp);
	CDEBUG_LOG(" sGetPayFailUrl url:[%s] strGetBankNoRsp : %s\n", mainConfig.sGetPayFailUrl.c_str(), strGetPayFailRsp.c_str());
	cJSON* root = cJSON_Parse(strGetPayFailRsp.c_str());
	cJSON* error  = cJSON_GetObjectItem(root, "error");
	if(error->valueint != -1)
	{
		cJSON * data = cJSON_GetObjectItem(root, "data");
		cJSON * total = cJSON_GetObjectItem(data, "total");
		CDEBUG_LOG("total value[%d]\n", total->valueint);
		if (total->valueint)
		{
			cJSON * lists = cJSON_GetObjectItem(data, "lists");
			int iTotal = cJSON_GetArraySize(lists);
			for (int i = 0; i < iTotal; ++i)
			{
				cJSON* tmp = cJSON_GetArrayItem(lists, i);
				JsonType obj = JsonUtil::json2obj(tmp);
				JsonMap jMap = obj.toMap();
				/*for (JsonMap::iterator iter = jMap.begin(); iter != jMap.end(); ++iter)
				{
					CDEBUG_LOG("i[%d] map key:[%s] value:[%s]\n", i, iter->first.toString().c_str(), iter->second.toString().c_str());
				}*/
				TRemitBill remitBill;
				remitBill.Reset();
				remitBill.account_id = jMap["accountid"].toString() ;
				remitBill.remit_fee = atoi(jMap["payed_amount"].toString().c_str());
				remitBill.fremit_fee = (float)remitBill.remit_fee / (float)100;
				remitBill.sPayTime =  jMap["dateflag"].toString();
				remitBill.sRemitTime = toDate(getSysDate());
				remitBill.sRemark =  jMap["remark"].toString();
				remitBill.sBankCardNo = jMap["bank_cardno"].toString();
				remitBill.sBankOwner = jMap["bank_owner"].toString();
				remitBill.sBankCardType = jMap["is_public"].toString();  //add hawrk
				remitBill.sBankType     = jMap["bank_type"].toString();   //add 银行简称
				remitBill.sBranchNo     = jMap["branch_no"].toString();   //add 网点号
				remitBill.sName = jMap["account_name"].toString();
				remitBill.sType = jMap["type"].toString();
				tremitMap.insert(std::make_pair(iIndex, remitBill));
				++iIndex;
			}
		}
	}

	CDEBUG_LOG("iIndex[%d] tremitMap size[%d]\n", iIndex, tremitMap.size());

	return 0;
}

/**
* brief:
* param:
* out:
* return: succ:RET_SUCCESS_UNIX
* */
int CSpeedPosServer::CallAddSettleLogApi(const std::string& strBmId, const std::string& sPayChannel, const TRemitBill& remitBill)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	time_t tNow = time(NULL);
	//load config
	CBillBusiConfig* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;

	StringMap addSettleMap;
	addSettleMap.insert(StringMap::value_type("_nonce_str", toString(tNow)));
	addSettleMap.insert(StringMap::value_type("_timestamp", toString(tNow)));
	addSettleMap.insert(StringMap::value_type("_version", "1.0"));
	addSettleMap.insert(StringMap::value_type("_datatype", "single_json"));
	addSettleMap.insert(StringMap::value_type("accountid", toString(remitBill.account_id)));
	addSettleMap.insert(StringMap::value_type("bank_cardno", remitBill.sBankCardNo));
	addSettleMap.insert(StringMap::value_type("bank_owner", remitBill.sBankOwner));
	addSettleMap.insert(StringMap::value_type("is_public",remitBill.sBankCardType));  //add 对公对私
	addSettleMap.insert(StringMap::value_type("bank_type",remitBill.sBankType));  //add 银行类型
	addSettleMap.insert(StringMap::value_type("branch_no",remitBill.sBranchNo));   //add 银行网点号
	addSettleMap.insert(StringMap::value_type("dateflag", remitBill.sPayTime));
	addSettleMap.insert(StringMap::value_type("paydate", remitBill.sRemitTime));
	addSettleMap.insert(StringMap::value_type("account_name", remitBill.sName));
	addSettleMap.insert(StringMap::value_type("payed_amount", toString(remitBill.remit_fee)));
	addSettleMap.insert(StringMap::value_type("remark", remitBill.sRemark));
	addSettleMap.insert(StringMap::value_type("bm_id", strBmId));
	addSettleMap.insert(StringMap::value_type("pay_channel", sPayChannel));
	addSettleMap.insert(StringMap::value_type("status", "-1"));
	addSettleMap.insert(StringMap::value_type("type", remitBill.sType));
	std::string strRsp = "";
	SendRequestDataApi(mainConfig.sApiKey, mainConfig.sAddSettleLogUrl, addSettleMap, strRsp);
	CDEBUG_LOG("sAddSettleLogUrl = %s strRsp = %s\n", mainConfig.sAddSettleLogUrl.c_str(), strRsp.c_str());
	cJSON* root = cJSON_Parse(strRsp.c_str());
	cJSON * error = cJSON_GetObjectItem(root, "error");
	CDEBUG_LOG("error = %d \n", error->valueint);
	iRet = error->valueint;
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "CallAddSettleLogApi failed ! "
			"Ret[%d].",
			iRet);
		CERROR_LOG("CallAddSettleLogApi failed! "
			"Ret[%d].\n",
			iRet);
		return iRet;
	}
	return 0;
}


/**
* brief:
* param:
* out:
* return: succ:RET_SUCCESS_UNIX
* */
int CSpeedPosServer::CallUpdateSettleLogApi(const std::string& strBmId, const std::string& sPayChannel, const TRemitBill& remitBill, const int& status)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	time_t tNow = time(NULL);
	//load config
	CBillBusiConfig* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;

	StringMap addSettleMap;
	addSettleMap.insert(StringMap::value_type("_nonce_str", toString(tNow)));
	addSettleMap.insert(StringMap::value_type("_timestamp", toString(tNow)));
	addSettleMap.insert(StringMap::value_type("_version", "1.0"));
	addSettleMap.insert(StringMap::value_type("_datatype", "single_json"));
	addSettleMap.insert(StringMap::value_type("bm_id", strBmId));
	addSettleMap.insert(StringMap::value_type("pay_channel", sPayChannel));
	addSettleMap.insert(StringMap::value_type("accountid", remitBill.account_id));
	addSettleMap.insert(StringMap::value_type("bank_cardno", remitBill.sBankCardNo));
	addSettleMap.insert(StringMap::value_type("bank_owner", remitBill.sBankOwner));
	addSettleMap.insert(StringMap::value_type("type", remitBill.sType));
	addSettleMap.insert(StringMap::value_type("dateflag", remitBill.sPayTime));
	addSettleMap.insert(StringMap::value_type("paydate", remitBill.sRemitTime));
	addSettleMap.insert(StringMap::value_type("is_public",remitBill.sBankCardType)); //add
	addSettleMap.insert(StringMap::value_type("bank_type",remitBill.sBankType)); //add
	addSettleMap.insert(StringMap::value_type("branch_no",remitBill.sBranchNo));  //add


	addSettleMap.insert(StringMap::value_type("status", toString(status)));

	addSettleMap.insert(StringMap::value_type("pay_remark",remitBill.sPayRemark));  //结算描述
	//addSettleMap.insert(StringMap::value_type("paytime",remitBill.sBranchNo)); //结算时间

	std::string strRsp = "";
	SendRequestDataApi(mainConfig.sApiKey, mainConfig.sUpdateSettleLogUrl, addSettleMap, strRsp);
	CDEBUG_LOG("sUpdateSettleLogUrl = %s strRsp = %s\n", mainConfig.sUpdateSettleLogUrl.c_str(), strRsp.c_str());
	cJSON* root = cJSON_Parse(strRsp.c_str());
	cJSON * error = cJSON_GetObjectItem(root, "error");
	iRet = error->valueint;
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "SendRequestDataApi failed ! "
			"Ret[%d].",
			iRet);
		CERROR_LOG("SendRequestDataApi failed! "
			"Ret[%d].\n",
			iRet);
		return -1010;
	}
	return 0;
}
/**
* brief:
* param:
* out:
* return: succ:RET_SUCCESS_UNIX
* */
int CSpeedPosServer::CallAddBillContrastApi(const std::string& strBmId, const std::string& sPayChannel,
								const std::string& strPayTime, const int& iStep, const std::string& strRemark)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	time_t tNow = time(NULL);
	//load config
	CBillBusiConfig* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;
	StringMap addBillContrastMap;
	addBillContrastMap.insert(StringMap::value_type("_nonce_str", toString(tNow)));
	addBillContrastMap.insert(StringMap::value_type("_timestamp", toString(tNow)));
	addBillContrastMap.insert(StringMap::value_type("_version", "1.0"));

	addBillContrastMap.insert(StringMap::value_type("bm_id", strBmId));
	addBillContrastMap.insert(StringMap::value_type("dateflag", strPayTime));
	addBillContrastMap.insert(StringMap::value_type("pay_channel", sPayChannel));

	addBillContrastMap.insert(StringMap::value_type("paydate", toDate(getSysDate())));
	addBillContrastMap.insert(StringMap::value_type("step", toString(iStep)));
	addBillContrastMap.insert(StringMap::value_type("remark", strRemark));
	addBillContrastMap.insert(StringMap::value_type("status", toString(-2)));
	std::string strRsp = "";
	SendRequestDataApi(mainConfig.sApiKey, mainConfig.sAddBillContrastUrl, addBillContrastMap, strRsp);
	CDEBUG_LOG("sAddBillContrastUrl = %s strRsp = %s\n", mainConfig.sAddBillContrastUrl.c_str(), strRsp.c_str());
	cJSON* root = cJSON_Parse(strRsp.c_str());
	cJSON * error = cJSON_GetObjectItem(root, "error");
	iRet = error->valueint;
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "SendRequestDataApi failed ! "
			"Ret[%d].",
			iRet);
		CERROR_LOG("SendRequestDataApi failed! "
			"Ret[%d].\n",
			iRet);
		return -1010;
	}
	return 0;
}

/**
* brief:
* param:
* out:
* return: succ:RET_SUCCESS_UNIX
* */
int CSpeedPosServer::CallUpdateBillContrastApi(const string& strBmId, const std::string& sPayChannel, const std::string& strPayTime, const int& iStep,
								const std::string& strRemark, const int& iStatus /* = -2 */, const int& total_count /* = 0 */, const int& total_amount /* = 0 */)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	time_t tNow = time(NULL);
	//load config
	CBillBusiConfig* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;
	StringMap UpdateBillContrastMap;
	UpdateBillContrastMap.insert(StringMap::value_type("_nonce_str", toString(tNow)));
	UpdateBillContrastMap.insert(StringMap::value_type("_timestamp", toString(tNow)));
	UpdateBillContrastMap.insert(StringMap::value_type("_version", "1.0"));

	UpdateBillContrastMap.insert(StringMap::value_type("bm_id", strBmId));
	UpdateBillContrastMap.insert(StringMap::value_type("dateflag", strPayTime));
	UpdateBillContrastMap.insert(StringMap::value_type("pay_channel", sPayChannel));
	if (0 != iStep)
	{
		UpdateBillContrastMap.insert(StringMap::value_type("step", toString(iStep)));
	}
	if (!strRemark.empty())
	{
		UpdateBillContrastMap.insert(StringMap::value_type("remark", strRemark));
	}
	UpdateBillContrastMap.insert(StringMap::value_type("status", toString(iStatus)));
	if (0 < total_amount)
	{
		UpdateBillContrastMap.insert(StringMap::value_type("total_amount", toString(total_amount)));
	}
	if (0 < total_count)
	{
		UpdateBillContrastMap.insert(StringMap::value_type("total_count", toString(total_count)));
	}
	std::string strRsp = "";
	SendRequestDataApi(mainConfig.sApiKey, mainConfig.sUpdateBillContrastUrl, UpdateBillContrastMap, strRsp);
	CDEBUG_LOG("sUpdateBillContrastUrl = %s strRsp = %s\n", mainConfig.sUpdateBillContrastUrl.c_str(), strRsp.c_str());
	cJSON* root = cJSON_Parse(strRsp.c_str());
	cJSON * error = cJSON_GetObjectItem(root, "error");
	iRet = error->valueint;
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "SendRequestDataApi failed ! "
			"Ret[%d].",
			iRet);
		CERROR_LOG("SendRequestDataApi failed! "
			"Ret[%d].\n",
			iRet);
		return -1010;
	}
	return 0;
}

/**
* brief:
* param:
* out:
* return: succ:RET_SUCCESS_UNIX
* */
int CSpeedPosServer::CallWxpayExceptionOrderMsgApi(const std::string& strBmId, const int& problem_type, const WxFlowSummary& wxFlow)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	time_t tNow = time(NULL);
	//load config
	CBillBusiConfig* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;

	StringMap addExceptionMap;
	addExceptionMap.insert(StringMap::value_type("_nonce_str", toString(tNow)));
	addExceptionMap.insert(StringMap::value_type("_timestamp", toString(tNow)));
	addExceptionMap.insert(StringMap::value_type("_version", "1.0"));


	addExceptionMap.insert(StringMap::value_type("bm_id",strBmId));
	addExceptionMap.insert(StringMap::value_type("trade_time", toString(toUnixTime(wxFlow.pay_time))));
	addExceptionMap.insert(StringMap::value_type("order_no", wxFlow.order_no));
	addExceptionMap.insert(StringMap::value_type("transaction_id", wxFlow.transaction_id));
	addExceptionMap.insert(StringMap::value_type("pay_channel", "WXPAY"));
	addExceptionMap.insert(StringMap::value_type("order_type", wxFlow.order_status));
	addExceptionMap.insert(StringMap::value_type("problem_type", toString(problem_type)));
	if (0 == strcmp(wxFlow.order_status.c_str(), "REFUND"))
	{
		addExceptionMap.insert(StringMap::value_type("refund_no", wxFlow.refund_no));
		addExceptionMap.insert(StringMap::value_type("refund_id", wxFlow.refund_id));
	}
	std::string strRsp = "";
	SendRequestDataApi(mainConfig.sApiKey, mainConfig.sAddExceptionOrderUrl, addExceptionMap, strRsp);
	CDEBUG_LOG("sAddExceptionOrderUrl = %s strRsp = %s\n", mainConfig.sAddExceptionOrderUrl.c_str(), strRsp.c_str());
	cJSON* root = cJSON_Parse(strRsp.c_str());
	cJSON * error = cJSON_GetObjectItem(root, "error");
	iRet = error->valueint;
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "SendRequestDataApi failed ! "
			"Ret[%d].",
			iRet);
		CERROR_LOG("SendRequestDataApi failed! "
			"Ret[%d].\n",
			iRet);
		return -1010;
	}

	return 0;
}


int CSpeedPosServer::CallAlipayExceptionOrderMsgApi(const std::string& strBmId, const int& problem_type, const AliFlowSummary& aliFlow)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	time_t tNow = time(NULL);
	//load config
	CBillBusiConfig* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;

	StringMap addExceptionMap;
	addExceptionMap.insert(StringMap::value_type("_nonce_str", toString(tNow)));
	addExceptionMap.insert(StringMap::value_type("_timestamp", toString(tNow)));
	addExceptionMap.insert(StringMap::value_type("_version", "1.0"));


	addExceptionMap.insert(StringMap::value_type("bm_id", strBmId));
	addExceptionMap.insert(StringMap::value_type("trade_time", toString(toUnixTime(aliFlow.pay_time))));
	addExceptionMap.insert(StringMap::value_type("order_no", aliFlow.order_no));
	addExceptionMap.insert(StringMap::value_type("transaction_id", aliFlow.transaction_id));
	addExceptionMap.insert(StringMap::value_type("pay_channel", ALI_API_PAY_CHANNEL));
	addExceptionMap.insert(StringMap::value_type("order_type", aliFlow.order_status));
	addExceptionMap.insert(StringMap::value_type("problem_type", toString(problem_type)));
	if (0 == strcmp(aliFlow.order_status.c_str(), "REFUND"))
	{
		addExceptionMap.insert(StringMap::value_type("refund_no", aliFlow.refund_no));
	}
	std::string strRsp = "";
	SendRequestDataApi(mainConfig.sApiKey, mainConfig.sAddExceptionOrderUrl, addExceptionMap, strRsp);
	CDEBUG_LOG("sAddExceptionOrderUrl = %s strRsp = %s\n", mainConfig.sAddExceptionOrderUrl.c_str(), strRsp.c_str());
	cJSON* root = cJSON_Parse(strRsp.c_str());
	cJSON * error = cJSON_GetObjectItem(root, "error");
	iRet = error->valueint;
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg), "SendRequestDataApi failed ! "
			"Ret[%d].",
			iRet);
		CERROR_LOG("SendRequestDataApi failed! "
			"Ret[%d].\n",
			iRet);
		return -1010;
	}

	return 0;
}


int CSpeedPosServer::ProcSftpDownLoad(const char * user, const char * pwd, const char * ipaddr, int port, const char * filepath, CBillFile& pFile)

{
	int socket_fd, i, auth_pw = 0;
	CDEBUG_LOG("user[%s] pwd[%s] ipaddr[%s] port[%d] filepath[%s]!",
		user, pwd, ipaddr, port, filepath);
	struct sockaddr_in sin;
	const char *fingerprint;
	char *userauthlist;
	LIBSSH2_SESSION *session;
	int rc;
	LIBSSH2_SFTP *sftp_session;
	LIBSSH2_SFTP_HANDLE *sftp_handle;

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	if (inet_pton(AF_INET, ipaddr, &sin.sin_addr) <= 0)
		return -1;
	if (connect(socket_fd, (struct sockaddr*)(&sin),
		sizeof(struct sockaddr_in)) != 0) {
		//fprintf(stderr, "failed to connect!\n");
		CERROR_LOG("failed to connect!\n");
		return -1;
	}

	/* Create a session instance
	*/
	session = libssh2_session_init();

	if (!session)
		return -1;

	/* Since we have set non-blocking, tell libssh2 we are blocking */
	libssh2_session_set_blocking(session, 1);


	/* ... start it up. This will trade welcome banners, exchange keys,
	* and setup crypto, compression, and MAC layers
	*/
	rc = libssh2_session_handshake(session, socket_fd);

	if (rc) {
		CERROR_LOG("Failure establishing SSH session: %d\n");
		return -1;
	}

	fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);

	fprintf(stderr, "Fingerprint: ");
	for (i = 0; i < 20; i++) {
		fprintf(stderr, "%02X ", (unsigned char)fingerprint[i]);
	}
	fprintf(stderr, "\n");

	/* check what authentication methods are available */
	userauthlist = libssh2_userauth_list(session, user, strlen(user));
	CDEBUG_LOG("Authentication methods : %s\n", userauthlist);
	if (strstr(userauthlist, "password") != NULL) {
		auth_pw |= 1;
	}
	if (strstr(userauthlist, "keyboard-interactive") != NULL) {
		auth_pw |= 2;
	}
	if (strstr(userauthlist, "publickey") != NULL) {
		auth_pw |= 4;
	}
	auth_pw = 1;

	if (auth_pw & 1) {
		/* We could authenticate via password */
		if (libssh2_userauth_password(session, user, pwd)) {
			CDEBUG_LOG("Authentication by password failed.\n");
			libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");

			libssh2_session_free(session);

			close(socket_fd);

			return -101;
		}
	}
	else {
		CDEBUG_LOG("No supported authentication methods found!\n");
		libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");

		libssh2_session_free(session);

		close(socket_fd);

		return -102;
		//goto shutdown;
	}

	CDEBUG_LOG("libssh2_sftp_init()!\n");

	sftp_session = libssh2_sftp_init(session);


	if (!sftp_session) {
		CDEBUG_LOG("Unable to init SFTP session\n");
		libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");

		libssh2_session_free(session);

		close(socket_fd);

		return -101;
		//goto shutdown;
	}

	CDEBUG_LOG("libssh2_sftp_open() filepath[%s]!\n", filepath);

	/* Request a file via SFTP */
	sftp_handle = libssh2_sftp_open(sftp_session, filepath, LIBSSH2_FXF_READ, 0);


	if (!sftp_handle) {

		CDEBUG_LOG("Unable to open file with SFTP: %ld\n",
			libssh2_sftp_last_error(sftp_session));

		libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");

		libssh2_session_free(session);

		close(socket_fd);

		return -1;
	}

	CDEBUG_LOG("libssh2_sftp_open() is done, now receive data!\n");
	do {

		char mem[1024];

		/* loop until we fail */
		//CDEBUG_LOG("libssh2_sftp_read()!\n");

		rc = libssh2_sftp_read(sftp_handle, mem, sizeof(mem));
		if (rc > 0) {
			//write(1, mem, rc);
			pFile._write(mem, rc);
		}
		else {
			break;
		}


	} while (1);

	libssh2_sftp_close(sftp_handle);

	libssh2_sftp_shutdown(sftp_session);


//shutdown:

	libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");

	libssh2_session_free(session);

	close(socket_fd);

	fprintf(stderr, "all done\n");

	libssh2_exit();

	return 0;
}

/**
* brief:
* param:
* out:
* return: succ:RET_SUCCESS_UNIX
* */
const char* CSpeedPosServer::GetMd5(const char* szSrc, size_t size, char* szBuf)
{
	unsigned char szMd5[16];
	MD5(reinterpret_cast<const unsigned char*>(szSrc), size, szMd5);
	for (int i = 0; i < 16; ++i)
	{
		sprintf(&(szBuf[i * 2]), "%02x", (unsigned int)(szMd5[i]));
		szBuf[i * 2] = toupper(szBuf[i * 2]);
		szBuf[i * 2 + 1] = toupper(szBuf[i * 2 + 1]);
	}

	return szBuf;
}

string  CSpeedPosServer::getSysTimeEx(time_t t)
{
    struct  tm tm_now;
    localtime_r(&t, &tm_now);

    char szTmp[256];
    snprintf(szTmp, sizeof(szTmp), "%04d-%02d-%02d %02d:%02d:%02d",
            tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
            tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);

    return szTmp;
}

