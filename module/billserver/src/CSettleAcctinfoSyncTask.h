/*
 * CSettleAcctinfoSyncTask.h
 * 
 *  Created on: 2017-06-23
 *      Author: 
 */

#ifndef _C_SETTLE_ACCTINFO_SYNC_TASK_H_
#define _C_SETTLE_ACCTINFO_SYNC_TASK_H_

#include "IUrlProtocolTask.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../Base/Comm/comm_protocol.h"
#include "CExp.h"
#include <sstream>
#include "mysqlapi.h"
#include "CSpeedPosConfig.h"
#include "../Base/Comm/clib_mysql.h"
#include "CheckParameters.h"
#include "json_util.h"

using namespace std;

class CSettleAcctinfoSyncTask : public IUrlProtocolTask
{
public:
	CSettleAcctinfoSyncTask(){}
	virtual ~CSettleAcctinfoSyncTask(){}

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
		m_pBillDB = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
		m_retJsonLists.clear();
    }

protected:
    void FillReq( NameValueMap& mapInput);
    void CheckInput();
    void BuildResp( CHAR** outbuf, INT32& outlen );

	void Deal();

	void SetRetParam();

	void SyncSettleAcctinfo();

    /** 用来上报统计 */
    struct timeval m_stStart;
    struct timeval m_stEnd;

	NameValueMap m_InParams;
	NameValueMap m_RetMap;
	JsonMap m_ContentJsonMap;

	CMySQL m_mysql;

	clib_mysql * m_pBillDB = NULL;
	JsonList m_retJsonLists;
};


#endif

