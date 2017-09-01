/*
 * CAccountChecking.h
 *
 *  Created on: 2017年8月18日
 *      Author: hawrkchen
 *      Desc :清分入账操作（渠道和商户）
 */

#ifndef _CACCOUNTCHECKING_H_
#define _CACCOUNTCHECKING_H_

#include "IUrlProtocolTask.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../Base/Comm/clib_mysql.h"
#include "../Base/common/comm_tools.h"
#include "CheckParameters.h"
#include "CRouteBillBase.h"


class CAccountChecking : public IUrlProtocolTask
{
public:
	CAccountChecking();
	virtual ~CAccountChecking();

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
		m_pBillDB = Singleton<CSpeedPosConfig>::GetInstance()->GetBaseDB();
    }

protected:
    void FillReq( NameValueMap& mapInput);
    void CheckInput();
    void BuildResp( CHAR** outbuf, INT32& outlen );

    INT32 CalcEffectiveTimeBill();

    void CheckAccountStatus();

    std::string GetAccountSeqNo();

    void CheckinAccount();


    void UpdateDistributionStatus(const string& account_no,const string& acc_status,const string& acc_desc);

	void SetRetParam();

	std::string m_start_time;
	std::string m_end_time;

	NameValueMap m_InParams;
	NameValueMap m_RetMap;
	JsonMap m_ContentJsonMap;

	std::map<std::string, MchBillSum> mchPayBillMap;
	std::map<std::string, MchBillSum> mchRefundBillMap;

	CMySQL m_mysql;

	ostringstream sqlss;

	clib_mysql * m_pBillDB = NULL;

};



#endif /* _CACCOUNTCHECKING_H_ */
