/*
 * CProcSettleCallback.h
 *
 *  Created on: 2017年4月19日
 *      Author: hawrkchen
 *      Desc:
 */

#ifndef _CPROCSETTLECALLBACK_H_
#define _CPROCSETTLECALLBACK_H_

#include "IUrlProtocolTask.h"
#include "CSpeedPosConfig.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "CBillFile.h"
#include "error.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

class CProcSettleCallback : public IUrlProtocolTask
{
public:
	CProcSettleCallback();
	virtual ~CProcSettleCallback();

    INT32 Init()
    {
        m_bInited = true;
        return 0;
    }

	INT32 CallSettle();

	INT32 HandleProcess();

    INT32 Execute( NameValueMap& mapInput, char** outbuf, int& outlen );

    void LogProcess();
    void Reset()
    {
        IUrlProtocolTask::Reset();

		m_stResp.Reset();
    }

protected:
    INT32 FillReq( NameValueMap& mapInput);
    INT32 CheckInput();
    void BuildResp( CHAR** outbuf, INT32& outlen );


	ProPullBillReq m_stReq;
	BillServerResponse m_stResp;

	CBillBusiConfig* pBillBusConfig;
	CBillFile* pSettleFille;
	CBillFile* pBankResultfile;

};



#endif /* _CPROCBILLEXCEPTIONTASK_H_ */
