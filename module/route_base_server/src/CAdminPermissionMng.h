/*
 * CAdminPermissionMng.h
 *
 *  Created on: 2017年7月11日
 *      Author: hawrkchen
 *      Desc:权限控制表管理
 */

#ifndef _CADMINPERMISSIONMNG_H_
#define _CADMINPERMISSIONMNG_H_


#include "IUrlProtocolTask.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../Base/Comm/clib_mysql.h"
#include "CheckParameters.h"
#include "CRouteBase.h"


class CAdminPermissionMsg : public IUrlProtocolTask
{
public:
	CAdminPermissionMsg(){}
	virtual ~CAdminPermissionMsg(){}

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
		m_subuserInfo.clear();
    }

protected:
    void FillReq( NameValueMap& mapInput);
    void CheckInput();
    void BuildResp( CHAR** outbuf, INT32& outlen );

	void AddAdminPermissionDB();

	void EditAdminPermissionDB();

	void QueryAdminPermissionDB();

	void SetRetParam();

    /** 用来上报统计 */
//    struct timeval m_stStart;
//    struct timeval m_stEnd;

	NameValueMap m_InParams;
	NameValueMap m_RetMap;
	JsonMap m_ContentJsonMap;

	CMySQL m_mysql;

	ostringstream sqlss;

	clib_mysql * m_pBaseDB = NULL;

	SqlResultSet m_subuserInfo;
};


#endif /* _CADMINPERMISSIONMNG_H_ */
