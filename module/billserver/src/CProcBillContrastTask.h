/*
 * CCreateOrderTask.h
 * 创建订单接口，生成团购凭证，返回订单号
 *  Created on: 2010-5-20
 *      Author: rogeryang
 */

#ifndef _C_PROC_BILL_CONTRAST_TASK_H
#define _C_PROC_BILL_CONTRAST_TASK_H

#include "IUrlProtocolTask.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "error.h"
#include "../../Base/Comm/UserInfoClient.h"
#include "json_util.h"
#include <sys/mman.h>
#include <stdlib.h>
#include "CBillConstrastBase.h"
#include "BankFactory.h"


class CProcBillContrastTask : public IUrlProtocolTask
{
public:
	CProcBillContrastTask();

	virtual ~CProcBillContrastTask(){}

    INT32 Init()
    {
        m_bInited = true;
        return 0;
    }

	INT32 CalcEffectiveTimeBill();

    INT32 Execute( NameValueMap& mapInput, char** outbuf, int& outlen );


    void LogProcess();

protected:
    INT32 FillReq( NameValueMap& mapInput);
    INT32 CheckInput();
    void BuildResp( CHAR** outbuf, INT32& outlen );

    ErrParamMap errMap;

	ProPullBillReq m_stReq;
	BillServerResponse m_stResp;

	int m_iBillBeginTime;
	int m_iBillEndTime;

};

#endif /* CCREATEORDER_TASK_H_ */
