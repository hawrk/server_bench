/*
 * CRedisClient.h
 *
 *  Created on: 2012-12-12
 *      Author: sangechen
 */

#ifndef _C_SZ_SPD_API_H_
#define _C_SZ_SPD_API_H_

#include <string.h>
#include <string>
#include "tinyxml2/tinyxml2.h"
#include "error.h"
//#include "http/http_client.h"
#include "comm_tools.h"
#include "exception.h"
#include "tools.h"
#include "CPayApiBase.h"
#include "CCurlClient.h"
#include "CRoutePayGateProtocol.h"
#include <time.h>


class  CSzSPDApi : public CPayApiBase
{

public:
	CSzSPDApi();

	virtual ~CSzSPDApi();

	/*
	* @brief 商户进件
	*/
	virtual int AddShop(StringMap& inReq, StringMap& outRsp);

	/*
	* @brief 商户下单
	*/
	virtual int Unifiedorder(StringMap& inReq, StringMap& outRsp);

	virtual int Micropay(StringMap& inReq, StringMap& outRsp);

	virtual int OrderQuery(StringMap& inReq, StringMap& outRsp);

	virtual int Refund(StringMap& inReq, StringMap& outRsp);

	virtual int RefundQuery(StringMap& inReq, StringMap& outRsp);

	virtual int CloseOrder(StringMap& inReq, StringMap& outRsp);

	/*
	* @brief 报备商户
	*/
	void ReportShop(const std::string& channel_mch_id, const std::string& payWay, 
		StringMap& inReq, std::string& payWaySubMchId);

	void UploadPic(const std::string& channel_mch_id, const std::string jsonKey, 
		const std::string& agentId, const std::string& picUrl);

	void Reset();

private:

	std::string GetRequestNo();

	void SendMsgToPay(const std::string& url, 
					const std::string& reqSignSrc, 
					const std::string& idStr,
					std::string& szResBody, int type = 0);

	std::string CreateAgentSign(const string& idStr, const string& plainText, int type = 0);

	std::string CreatePicSign(const string& agentId, const string& plainText);

	std::string MakeSignSrc(StringMap& inMap);


private:

	std::string base_url_;

	std::string version_;

	std::string base_beta_url_;

	struct timeval stStart;

	struct timeval stEnd;
};

#endif /* REDISCLIENT_H_ */
