/*
 * CDownLoadSettleFile.h
 *
 *  Created on: 2017年7月19日
 *      Author: hawrkchen
 *      Desc: 结算单下载
 */

#ifndef _CDOWNLOADSETTLEFILE_H_
#define _CDOWNLOADSETTLEFILE_H_

#include "IUrlProtocolTask.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../Base/Comm/clib_mysql.h"
#include "../Base/common/comm_tools.h"
#include "CheckParameters.h"
#include "CRouteBillBase.h"


class CDownLoadSettleFile : public IUrlProtocolTask
{
public:
	CDownLoadSettleFile(){}
	virtual ~CDownLoadSettleFile(){}

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

    void CallPayGate2DownSettleBill();

	void SetRetParam();

	NameValueMap m_InParams;
	NameValueMap m_RetMap;
	JsonMap m_ContentJsonMap;

	CMySQL m_mysql;

	ostringstream sqlss;

	clib_mysql * m_pBaseDB = NULL;

};




#endif /* _CDOWNLOADSETTLEFILE_H_ */
