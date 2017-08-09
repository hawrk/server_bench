/*
 * CDownLoadCheckBill.h
 *
 *  Created on: 2017年7月19日
 *      Author: hawrkchen
 *      Desc: 对账单下载
 */

#ifndef _CDOWNLOADCHECKBILL_H_
#define _CDOWNLOADCHECKBILL_H_

#include "IUrlProtocolTask.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../Base/Comm/clib_mysql.h"
#include "../Base/common/comm_tools.h"
#include "CheckParameters.h"
#include "CRouteBillBase.h"


class CDownLoadCheckBill : public IUrlProtocolTask
{
public:
	CDownLoadCheckBill(){}
	virtual ~CDownLoadCheckBill(){}

    INT32 Init()
    {
        m_bInited = true;
        return 0;
    }

	void LogProcess();

    INT32 Execute( NameValueMap& mapInput, char** outbuf, int& outlen );

    void Reset()
    {
        IUrlProtocolTask::Reset();
		m_InParams.clear();
		m_RetMap.clear();
		m_ContentJsonMap.clear();
		m_pBaseDB = Singleton<CSpeedPosConfig>::GetInstance()->GetBaseDB();
    }

protected:
    void FillReq( NameValueMap& mapInput);
    void CheckInput();
    void BuildResp( CHAR** outbuf, INT32& outlen );

    void CallPayGate2GetFactorID();

    void CreateMsgBody(const string& factor_id);

    void CreateSwiftMsgBody(StringMap& paramMap,const string& factor_id);

    string CreateRSASign(const string& idStr, const string& plainText);

    string CreateMD5Sign(const string& factor_id, const string& plainText);

    void SendMsgToBank( const std::string& factor_id, std::string& szResBody);

    void SendMsgToSwift(const StringMap& paramMap,const std::string& factor_id, std::string& szResBody);

	void SetRetParam();


	NameValueMap m_InParams;
	NameValueMap m_RetMap;
	JsonMap m_ContentJsonMap;

	vector<string> factor_vec;
	string m_reqMsg;
	string m_sendUrl;

	CMySQL m_mysql;

	ostringstream sqlss;

	clib_mysql * m_pBaseDB = NULL;

};




#endif /* _CDOWNLOADCHECKBILL_H_ */
