/*
 * CExceptionProc.h
 *
 *  Created on: 2017年8月21日
 *      Author: hawrkchen
 *      Desc:对账差错处理状态处理
 */

#ifndef _CEXCEPTIONPROC_H_
#define _CEXCEPTIONPROC_H_

#include "IUrlProtocolTask.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../Base/Comm/clib_mysql.h"
#include "../Base/common/comm_tools.h"
#include "CheckParameters.h"
#include "CRouteBillBase.h"


class CExceptionProc : public IUrlProtocolTask
{
public:
	CExceptionProc();
	virtual ~CExceptionProc();

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

    void CheckOrderStatus();

    void UpdateAbnormalStatus(int type);

    INT32 OrderSync(string& order_no);

	void SetRetParam();

	NameValueMap m_InParams;
	NameValueMap m_RetMap;
	JsonMap m_ContentJsonMap;

	CMySQL m_mysql;

	ostringstream sqlss;

	clib_mysql * m_pBillDB = NULL;

};



#endif /* _CEXCEPTIONPROC_H_ */
