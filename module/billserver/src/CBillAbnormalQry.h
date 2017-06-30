/*
 * CBillAbnormalQry.h
 *
 *  Created on: 2017年6月9日
 *      Author: hawrkchen
 *      Desc:对账差错查询
 */

#ifndef _CBILLABNORMALQRY_H_
#define _CBILLABNORMALQRY_H_


#include "IUrlProtocolTask.h"
#include "log/clog.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "error.h"
#include "../../Base/Comm/UserInfoClient.h"
#include "json_util.h"
#include "bill_protocol.h"
#include "speed_bill_protocol.h"
#include "CExp.h"
#include "CheckParameters.h"
#include "CSpeedPosConfig.h"

class CBillAbnormalQry : public IUrlProtocolTask
{
public:
	CBillAbnormalQry();

	virtual ~CBillAbnormalQry(){}

    INT32 Init()
    {
        m_bInited = true;
        return 0;
    }


    INT32 Execute( NameValueMap& mapInput, char** outbuf, int& outlen );

    void DealQueryAbnormal();


    void LogProcess();

protected:
    INT32 FillReq( NameValueMap& mapInput);
    INT32 CheckInput();
    void BuildResp( CHAR** outbuf, INT32& outlen );

    ErrParamMap errMap;


    NameValueMap m_InParams;

    JsonMap m_ContentJsonMap;
	//ProPullBillReq m_stReq;
	BillServerResponse m_stResp;


};


#endif /* _CBILLABNORMALQRY_H_ */
