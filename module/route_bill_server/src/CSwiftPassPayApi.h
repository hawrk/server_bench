/*
 * CRedisClient.h
 *
 *  Created on: 2012-12-12
 *      Author: sangechen
 */

#ifndef _C_SWIFT_PASS_API_CLIENT_H_
#define _C_SWIFT_PASS_API_CLIENT_H_

#include <string.h>
#include <string>
#include "tinyxml2/tinyxml2.h"
#include "error.h"
//#include "http/http_client.h"
#include "comm_tools.h"
#include "exception.h"
#include "CPayApiBase.h"
#include "CCurlClient.h"
#include "tools.h"
#include "CRoutePayGateProtocol.h"
#include "error.h"
#include "CSocket.h"
#include "CRoutePayGateConfig.h"

extern ErrParamMap errMap;

class  CSwiftPassPayApi : public CPayApiBase
{

public:
	CSwiftPassPayApi();
	virtual ~CSwiftPassPayApi();

	virtual int AddShop(StringMap& inReq, StringMap& outRsp);

	virtual int Unifiedorder(StringMap& inReq, StringMap& outRsp);

	virtual int Micropay(StringMap& inReq, StringMap& outRsp);

	virtual int OrderQuery(StringMap& inReq, StringMap& outRsp);

	virtual int Refund(StringMap& inReq, StringMap& outRsp);

	virtual int RefundQuery(StringMap& inReq, StringMap& outRsp);

	virtual int CloseOrder(StringMap& inReq, StringMap& outRsp);

	void Reset();

public: 

private:

	int SendMsgToApi(const std::string& mchId, const std::string& payChannelId, 
			const std::string& apiUrl, StringMap& paramMap, std::string& szResBody);

	string CreateAgentSign(const string& idStr, const std::string& payChannelId,
					const string& plainText);

	bool CheckAgentSign(const tinyxml2::XMLDocument* pDoc, const std::string& mchId, const std::string& payChannelId);

	struct timeval stStart;

	struct timeval stEnd;

	std::string base_url_;
};

#endif /* REDISCLIENT_H_ */
