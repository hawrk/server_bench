/*
 * CAgentPayBillLogQryTask.h
 * 
 *  Created on: 2010-5-20
 *      Author: 
 */

#ifndef _C_APAY_BILL_LOG_QRY_TASK_H_
#define _C_APAY_BILL_LOG_QRY_TASK_H_

#include "IUrlProtocolTask.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

#include "../../Base/Comm/comm_protocol.h"
#include "CExp.h"
#include <sstream>
#include "../common/mysqlapi.h"
#include "CAgentPayBase.h"
#include "CSpeedPosConfig.h"
#include "../../Base/Comm/clib_mysql.h"
#include "../common/CCommFunc.h"
#include "../common/CheckParameters.h"
#include "json_util.h"
#include "../business/apayErrorNo.h"

typedef map<string, SqlResultSet> StrSqlResultSetMap;


class CAPayBillLogQryTask : public IUrlProtocolTask
{
public:
	CAPayBillLogQryTask(){}
	virtual ~CAPayBillLogQryTask(){}

	void LogProcess();

    INT32 Init()
    {
        m_bInited = true;
        return 0;
    }

    INT32 Execute( NameValueMap& mapInput, char** outbuf, int& outlen );

    void Reset()
    {
        IUrlProtocolTask::Reset();
        m_istart = 0;
        m_iend = 0;
		m_iBillBeginTime = 0;
		m_iBillEndTime = 0;

		m_InParams.clear();
		m_RetMap.clear();
		m_resultMVet.clear();
		
		m_JsonList.clear();
		m_ContenJsonMap.clear();

		pBillBusConfig = NULL;

		m_DBConn = NULL;
    }

protected:
    void FillReq( NameValueMap& mapInput);
    void CheckInput();
    void BuildResp( CHAR** outbuf, INT32& outlen );
	void SetRetParam();

	//业务处理
	void Deal();

	//查询总数
	void QryTotal();

	//查列表
	void QryDeal();

    /** 用来上报统计 */
    struct timeval m_stStart;
    struct timeval m_stEnd;
	
    int m_istart;
    int m_iend;
	int m_iBillBeginTime;
	int m_iBillEndTime;

	CMySQL m_SqlHandle;
	clib_mysql* m_DBConn;

	NameValueMap m_InParams;
	NameValueMap m_RetMap;
	SqlResultMapVector m_resultMVet;

	CBillBusiConfig* pBillBusConfig;

	JsonList m_JsonList;
	JsonMap m_ContenJsonMap;	
};

#endif /* CCREATEORDER_TASK_H_ */

