/*
 * CBillAbnormalQuery.h
 *
 *  Created on: 2017年7月20日
 *      Author: hawrkchen
 *      Desc:对账异常查询
 */

#ifndef _CBILLABNORMALQUERY_H_
#define _CBILLABNORMALQUERY_H_


#include "IUrlProtocolTask.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../Base/Comm/clib_mysql.h"
#include "../Base/common/comm_tools.h"
#include "CheckParameters.h"
#include "CRouteBillBase.h"


class CRouteBillAbnormalQuery : public IUrlProtocolTask
{
public:
	CRouteBillAbnormalQuery();
	virtual ~CRouteBillAbnormalQuery();

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

    void QueryAbnormal();

	void SetRetParam();

	//int m_iBillBeginTime;
	//int m_iBillEndTime;

	std::string m_start_time;
	std::string m_end_time;


	NameValueMap m_InParams;
	NameValueMap m_RetMap;
	JsonMap m_ContentJsonMap;

	CMySQL m_mysql;

	ostringstream sqlss;

	clib_mysql * m_pBillDB = NULL;

};


#endif /* _CBILLABNORMALQUERY_H_ */
