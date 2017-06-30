/*
 * COrderServer.h
 *
 *  Created on: 2010-5-24
 *  Author: rogeryang
 */

#ifndef CSPEEDPOS_ORDERSERVER_H_
#define CSPEEDPOS_ORDERSERVER_H_

#include "CObject.h"
#include "CIdGenClient.h"
//#include "CUinRangeLock.h"
#include "utils.h"
#include  <time.h>
#include "CBillFile.h"
#include "tinyxml2.h"
#include "json_util.h"
#include "openssl/md5.h"
#include "CBillBusiConfig.h"
#include "error.h"
#include "common.h"
#include "openapi_client.h"
#include "bill_protocol.h"
#include "CSppClient.h"
#include "../../Base/Comm/tools.h"
#include "http_client.h"
#include "des.h"
#include "speed_bill_protocol.h"
#include "CExp.h"
#include "urlparammap.h"

using namespace qqtg;

class CSpeedPosServer : public CObject
{
    public:
        CSpeedPosServer();
        virtual ~CSpeedPosServer();

		INT32 Init(const CommServer& stTradeGenServer);


		int CallGetBankNoApi(TRemitBill& remitBill);

		int CallAddSettleLogApi(const std::string& strBmId, const std::string& sPayChannel, const TRemitBill& remitBill);

		int CallUpdateSettleLogApi(const std::string& strBmId, const std::string& sPayChannel, const TRemitBill& remitBill, const int& status);

		int CallWxpayExceptionOrderMsgApi(const std::string& strBmId, const int& problem_type,
						const WxFlowSummary& wxFlow);

		int CallAlipayExceptionOrderMsgApi(const std::string& strBmId, const int& problem_type,
					const AliFlowSummary& aliFlow);

		int CallGetPayFailApi(const std::string& strBmId, int& iIndex, const std::string& sPayChannel, const std::string& strPayTime,
			std::map<int, TRemitBill>& tremitMap);

		int CallAddBillContrastApi(const std::string& strBmId, const std::string& sPayChannel, const std::string& strPayTime,
					 const int& iStep, const std::string& strRemark);

		int CallUpdateBillContrastApi(const string& strBmId, const std::string& sPayChannel, const std::string& strPayTime,
							const int& iStep, const std::string& strRemark, const int& iStatus = -2, 
							const int& total_count = 0, const int& total_amount = 0);
		//CSpeedPosServer.cpp
		int SendRequestWxapi(const std::string& sign_key,
							const std::string& pay_ali_url, 
							StringMap& paramMap, 
							std::string& responseStr);

		int SendRequestDataApi(const std::string& sign_key,
						const std::string& pay_ali_url,
						StringMap& paramMap,
						std::string& responseStr);

		int SendRequestDownload(const std::string strUrl, std::string& path);

		int SendRequestTradeServapi(const std::string& sign_key,
											const std::string& trade_serv_url,
											StringMap& paramMap,
											std::string& responseStr);

		int ProcSftpDownLoad(const char * user,
			const char * pwd,
			const char * ipaddr,
			int port,
			const char * filepath,
			CBillFile& pFile);
	  
	  const char* GetMd5(const char* szSrc, size_t size, char* szBuf);

	  int SetOneFieldToXml(tinyxml2::XMLDocument * pDoc, tinyxml2::XMLNode* pXmlNode, const char * pcFieldName,
		  const char* pszValue, bool bIsCdata);

	  int SetOneFieldToXml(tinyxml2::XMLDocument * pDoc, tinyxml2::XMLNode* pXmlNode, const char * pcFieldName,
		  int32_t value, bool bIsCdata)
	  {
		  std::string strValue = toString(value);
		  return SetOneFieldToXml(pDoc, pXmlNode, pcFieldName, strValue.c_str(), bIsCdata);
	  }

	  int SetOneFieldToXml(tinyxml2::XMLDocument * pDoc, tinyxml2::XMLNode* pXmlNode, const char * pcFieldName,
		  int64_t value, bool bIsCdata)
	  {
		  std::string strValue = toString(value);
		  return SetOneFieldToXml(pDoc, pXmlNode, pcFieldName, strValue.c_str(), bIsCdata);
	  }

	  int SetOneFieldToXml(tinyxml2::XMLDocument * pDoc, tinyxml2::XMLNode* pXmlNode, const char * pcFieldName,
		  const std::string& strValue, bool bIsCdata)
	  {
		  return SetOneFieldToXml(pDoc, pXmlNode, pcFieldName, strValue.c_str(), bIsCdata);
	  }
	  
	  const char* GetXmlField(tinyxml2::XMLElement * xmlElement, const char* szField)
	  {
		  tinyxml2::XMLElement * msgElement = xmlElement->FirstChildElement(szField);
		  if (NULL == msgElement)
		  {
			  return NULL;
		  }

		  const char* pMsgBuf = msgElement->GetText();
		  return pMsgBuf;
	  }

	  std::string  getSysTimeEx(time_t t);

    public:
	  CSppClient sppClent;
	  CSppClient settleClient;

};

#endif /* CORDERSERVER_H_ */
