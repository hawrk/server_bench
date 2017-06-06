#include "openapi_client.h"
#include "common.h"
#include <string.h>



const string OpenapiClient::default_charset      = "utf-8";
const string OpenapiClient::default_url          = "https://openapi.alipay.com/gateway.do";
//const string OpenapiClient::default_url          =   "https://openapi.alipaydev.com/gateway.do";
const string OpenapiClient::default_sign_type    = "RSA";
const string OpenapiClient::default_version      = "1.0";

const string OpenapiClient::KEY_APP_ID           = "app_id";
const string OpenapiClient::KEY_METHOD           = "method";
const string OpenapiClient::KEY_CHARSET          = "charset";
const string OpenapiClient::KEY_SIGN_TYPE        = "sign_type";
const string OpenapiClient::KEY_SIGN             = "sign";
const string OpenapiClient::KEY_TIMESTAMP        = "timestamp";
const string OpenapiClient::KEY_VERSION          = "version";
const string OpenapiClient::KEY_BIZ_CONTENT      = "biz_content";


OpenapiClient::OpenapiClient(const string &appId,
                             const string &privateKey,
                             const string &url,
                             const string &charset,
                             const string &alipayPublicKey)
    : appId(appId),
      privateKey(privateKey),
      url(url),
      charset(charset),
      signType(default_sign_type),
      version(default_version),
      alipayPublicKey(alipayPublicKey) {

}

JsonMap OpenapiClient::invoke(const string &method, const JsonMap &contentMap, const StringMap &extendParamMap) {

    string content = JsonUtil::objectToString(JsonType(contentMap));
    string responseContent = invoke(method, content, extendParamMap);
	CDEBUG_LOG("OpenapiClient::invoke responseContent : %s\n", responseContent.c_str());
    JsonType jsonObj = JsonUtil::stringToObject(responseContent);
    return jsonObj.toMap();
}

string OpenapiClient::invoke(const string &method, const string &content, const StringMap &extendParamMap) {

    //time_t t = time(0);
    /*char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %X", localtime(&t));*/
	std::string strTmp = getSysTime();

	CDEBUG_LOG("OpenapiClient::invoke begin !\n");
    StringMap requestPairs;
    requestPairs.insert(StringMap::value_type(OpenapiClient::KEY_APP_ID, appId));
    requestPairs.insert(StringMap::value_type(OpenapiClient::KEY_BIZ_CONTENT, content));
    requestPairs.insert(StringMap::value_type(OpenapiClient::KEY_CHARSET, charset));
    requestPairs.insert(StringMap::value_type(OpenapiClient::KEY_METHOD, method));
    requestPairs.insert(StringMap::value_type(OpenapiClient::KEY_SIGN_TYPE, signType));
	requestPairs.insert(StringMap::value_type(OpenapiClient::KEY_TIMESTAMP, strTmp));
    requestPairs.insert(StringMap::value_type(OpenapiClient::KEY_VERSION, version));

    /** 追加外部传入的网关的补充参数，如notify_url等 **/
    for (StringMap::const_iterator iter = extendParamMap.begin(); iter != extendParamMap.end(); ++iter) {
        requestPairs.insert(StringMap::value_type(iter->first, iter->second));
    }

    string wholeContent = buildContent(requestPairs);
	CDEBUG_LOG("wholeContent : %s\n", wholeContent.c_str());
    string sign = OpenapiClient::rsaSign(wholeContent, privateKey);
	CDEBUG_LOG("sign : %s\n", sign.c_str());
    requestPairs.insert(StringMap::value_type(OpenapiClient::KEY_SIGN, sign));

    wholeContent = buildContent(requestPairs);
	CDEBUG_LOG("Request : %s\n", wholeContent.c_str());
    HttpClient httpClient;
    string responseStr = httpClient.sendSyncRequest(url, requestPairs);
	CDEBUG_LOG("Response : %s\n", responseStr.c_str());
    string responseContent = analyzeResponse(method, responseStr);

    return responseContent;
}

/**
 *
 * STL map default sort order by key
 *
 * STL map 默认按照key升序排列
 * 这里要注意如果使用的map必须按key升序排列
 *
 */
string OpenapiClient::buildContent(const StringMap &contentPairs) {

    string content;
    for (StringMap::const_iterator iter = contentPairs.begin();
         iter != contentPairs.end(); ++iter) {
        if (!content.empty()) {
            content.push_back('&');
        }
        content.append(iter->first);
        content.push_back('=');
        content.append(iter->second);
    }
    return content;
}

int OpenapiClient::TextToMapEx(const char* pchString, const char* pchSplitter, std::map<std::string , std::string>& mapResult)
{
	const char* pchStart, *pchEnd;
	int iSplitterLen = strlen(pchSplitter);

	mapResult.clear();

	if (pchString == NULL || pchSplitter == NULL || strlen(pchString) == 0 || strlen(pchSplitter) == 0)
	{
		return -1;
	}

	pchStart = pchString;
	pchEnd = strstr(pchStart, pchSplitter);

	while (pchEnd != NULL && (*pchStart) != '\0')
	{
		//获得一个片段(xxx=xxx)
		string strToken(pchStart, pchEnd - pchStart);
		string::size_type idx = strToken.find('=');
		if (string::npos == idx)
		{
			//没有=, 就存{"name", ""}
			mapResult.insert(make_pair(strToken, string("")));
		}
		else
		{
			//有=, 也要判断一下有没有value
			if (idx >= strToken.size() - 1)
			{
				//没有value
				string strName = strToken.substr(0, idx);
				string strValue = "";
				mapResult.insert(make_pair(strName, strValue));
			}
			else
			{
				string strName = strToken.substr(0, idx);
				string strValue = strToken.substr(idx + 1);
				//mapResult.insert( make_pair(StringUpper(strName), StringUpper(strValue)) );
				//不转换value的大小写
				char szDecodeValue[10240];
				clib_urldecode_comm(strValue.c_str(), szDecodeValue, sizeof(szDecodeValue));
				mapResult.insert(make_pair(strName, szDecodeValue));
			}

		}

		pchStart = pchEnd + iSplitterLen;
		if (pchStart != '\0')
		{
			pchEnd = strstr(pchStart, pchSplitter);
		}
	}

	return 0;
}

int OpenapiClient::clib_urldecode_comm(const char *as_src, char *as_des, const int ai_len)
{
	int  i = 0, j = 0;
	long l_asc = 0;
	char s_buf[10240] = { 0 };
	char s_tmp[10240] = { 0 };

	if (as_src == NULL
		|| as_des == NULL
		|| ai_len <= 0
		|| ai_len > 10240) {
		return(-1);
	} // if

	i = 0;
	j = 0;
	memset(s_buf, 0, sizeof(s_buf));
	while ((i < ai_len) && (as_src[i] != '\0') && (j < (int)sizeof(s_buf)-1)) {
		switch (as_src[i]) {
		case '%':
			snprintf(s_tmp, 3, "%s", as_src + i + 1);
			l_asc = strtol(s_tmp, NULL, 16);
			//s_buf[j] = toascii( l_asc );
			s_buf[j] = (char)l_asc;
			i += 2;
			break;
		case '+':
			s_buf[j] = ' ';
			break;
		default:
			s_buf[j] = as_src[i];
			break;
		} // switch

		i++;
		j++;
	} // while

	snprintf(as_des, ai_len, "%s", s_buf);
	return 0;
}

string OpenapiClient::analyzeResponse(const string &method, const string &responseStr)
{

    JsonType responseObj = JsonUtil::stringToObject(responseStr);
    JsonMap responseMap = responseObj.toMap();
	std::string rspHead = "";
    //获取返回报文中的alipay_xxx_xxx_response的内容;
	if (0 == strcmp(method.c_str(), "alipay.trade.create"))
	{
		rspHead = "alipay_trade_create_response\"";
	}
	else if (0 == strcmp(method.c_str(), "alipay.trade.refund"))
	{
		rspHead = "alipay_trade_refund_response\"";
	}
	else if (0 == strcmp(method.c_str(), "alipay.trade.fastpay.refund.query"))
	{
		rspHead = "alipay_trade_fastpay_refund_query_response\"";
	}
	else if (0 == strcmp(method.c_str(), "alipay.data.dataservice.bill.downloadurl.query"))
	{
		rspHead = "alipay_data_dataservice_bill_downloadurl_query_response\"";
	}
	int beg = responseStr.find(rspHead.c_str());
    int end = responseStr.rfind("\"sign\"");
    if (beg < 0 || end < 0) {
        return string();
    }
    beg = responseStr.find('{', beg);
    end = responseStr.rfind('}', end);
    //注意此处将map转为json之后的结果需要与支付宝返回报文中原格式与排序一致;
    //排序规则是节点中的各个json节点key首字母做字典排序;
    //Response的Json值内容需要包含首尾的“{”和“}”两个尖括号，双引号也需要参与验签;
    //如果字符串中包含“http://”的正斜杠，需要先将正斜杠做转义，默认打印出来的字符串是已经做过转义的;
    //此处转换之后的json字符串默认为"Compact"模式，即紧凑模式，不要有空格与换行;
    string responseContent = responseStr.substr(beg, end - beg + 1);

	CDEBUG_LOG("ResponseContent : %s \n", responseContent.c_str());
    //此处为校验支付宝返回报文中的签名;
    //如果支付宝公钥为空，则默认跳过该步骤，不校验签名;
    //如果支付宝公钥不为空，则认为需要校验签名;
  /*if (!alipayPublicKey.empty()) {

		LOG(INFO) << "AlipayPublicKey: " <<  alipayPublicKey.c_str();

		for (JsonMap::const_iterator::iterator it = responseMap.begin(); it != responseMap.end(); ++it)
		{
			LOG(INFO) << "type = " << it->second.type() << " key = " << it->first.toString() << " value = " << it->second.toString();
		}
		JsonMap::const_iterator iter = responseMap.find(OpenapiClient::KEY_SIGN);
        if (iter == responseMap.end()) {
			LOG(INFO) << "Cannot get Sign from response, Verify Failed";
            return string();
        }
        //获取返回报文中的sign;
        string responseSign = iter->second.toString();

		LOG(INFO) << "ResponseSign: " << responseSign.c_str();

        //调用验签方法;
        bool verifyResult = OpenapiClient::rsaVerify(responseContent, responseSign, alipayPublicKey);

        if (!verifyResult) {
			LOG(INFO) << "Verify Failed";
            return string();
        }
		LOG(INFO) << "Verify Success";
    } else {
		LOG(INFO) << "AlipayPublicKey is empty, Skip the Verify";
    }*/

    return responseContent;
}

string OpenapiClient::rsaSign(const string &content, const string &key) {

    string signed_str;
    const char *key_cstr = key.c_str();
	CDEBUG_LOG("key : [%s] \n", key_cstr);
    //int key_len = strlen(key_cstr);
	BIO *p_key_bio = NULL;
	p_key_bio = BIO_new(BIO_s_file());
	BIO_read_filename(p_key_bio, key_cstr);
    //BIO *p_key_bio = BIO_new_mem_buf((void *)key_cstr, key_len);
	//CDEBUG_LOG("p_key_bio : [%p] \n", p_key_bio);
	RSA *p_rsa = PEM_read_bio_RSAPrivateKey(p_key_bio, NULL, NULL, NULL);
    if (p_rsa != NULL) {

        const char *cstr = content.c_str();
        unsigned char hash[SHA_DIGEST_LENGTH] = {0};
        SHA1((unsigned char *)cstr, strlen(cstr), hash);
		//CDEBUG_LOG("cstr : [%s] hash [%s] \n", cstr, hash);
        unsigned char sign[XRSA_KEY_BITS / 8] = {0};
        unsigned int sign_len = sizeof(sign);
        int r = RSA_sign(NID_sha1, hash, SHA_DIGEST_LENGTH, sign, &sign_len, p_rsa);

        if (0 != r && sizeof(sign) == sign_len) {
            signed_str = base64Encode(sign, sign_len);
        }
    }

    RSA_free(p_rsa);
    BIO_free(p_key_bio);
    return signed_str;
}

bool OpenapiClient::rsaVerify(const string &content, const string &sign, const string &key) {

    bool result = false;
    const char *key_cstr = key.c_str();
    //int key_len = strlen(key_cstr);
   // BIO *p_key_bio = BIO_new_mem_buf((void *)key_cstr, key_len);
	BIO *p_key_bio = NULL;
	p_key_bio = BIO_new(BIO_s_file());
	CDEBUG_LOG("key : [%s] p_key_bio : [%p]\n", key.c_str(), p_key_bio);
	BIO_read_filename(p_key_bio, key_cstr);

    RSA *p_rsa = PEM_read_bio_RSA_PUBKEY(p_key_bio, NULL, NULL, NULL);

    if (p_rsa != NULL) {
        const char *cstr = content.c_str();
        unsigned char hash[SHA_DIGEST_LENGTH] = {0};
        SHA1((unsigned char *)cstr, strlen(cstr), hash);
        unsigned char sign_cstr[XRSA_KEY_BITS / 8] = {0};
        int len = XRSA_KEY_BITS / 8;
		//LOG(INFO) << "sign : " << sign;
		CDEBUG_LOG("sign : [%s] \n", sign.c_str());
        base64Decode(sign, sign_cstr, len);
        unsigned int sign_len = XRSA_KEY_BITS / 8;
        int r = RSA_verify(NID_sha1, hash, SHA_DIGEST_LENGTH, (unsigned char *)sign_cstr, sign_len, p_rsa);

        if (r > 0) {
            result = true;
        }
    }

    RSA_free(p_rsa);
    BIO_free(p_key_bio);
    return result;
}

string OpenapiClient::base64Encode(const unsigned char *bytes, int len) {

    BIO *bmem = NULL;
    BIO *b64 = NULL;
    BUF_MEM *bptr = NULL;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, bytes, len);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    string str = string(bptr->data, bptr->length);
    BIO_free_all(b64);
    return str;
}

bool OpenapiClient::base64Decode(const string &str, unsigned char *bytes, int &len) {

    const char *cstr = str.c_str();
    BIO *bmem = NULL;
    BIO *b64 = NULL;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new_mem_buf((void *)cstr, strlen(cstr));
    b64 = BIO_push(b64, bmem);
    len = BIO_read(b64, bytes, len);

    BIO_free_all(b64);
    return len > 0;
}

string OpenapiClient::getAppId() {
    return appId;
}

string OpenapiClient::getSignType() {
    return signType;
}

string OpenapiClient::getVersion() {
    return version;
}

string OpenapiClient::getCharset() {
    return charset;
}

string OpenapiClient::getUrl() {
    return url;
}

string OpenapiClient::getAlipayPublicKey() {
    return alipayPublicKey;
}
