/*
 * CLiquidationTask.h
 *
 *  Created on: 2017年7月27日
 *      Author: hawrkchen
 *      Desc:  清算接口
 */

#ifndef _CLIQUIDATIONTASK_H_
#define _CLIQUIDATIONTASK_H_

#include "IUrlProtocolTask.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../Base/Comm/clib_mysql.h"
#include "../Base/common/comm_tools.h"
#include "CheckParameters.h"
#include "CRouteBillBase.h"


class CLiquidationTask : public IUrlProtocolTask
{
public:
	CLiquidationTask();
	virtual ~CLiquidationTask();

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

    void CheckBillResult();

    void GetLiquidationData();

    void GetChannelData();

    void ProcLiquidation();

    void InsertLiquidationDB(const string& fund_type,OrderPayLiquidate& liq_map);

    void AccountCheckin();

	void SetRetParam();

	bool CheckBillSuccess(const string& order_no);


	std::string m_start_time;
	std::string m_end_time;


	//清分Map
	std::map<std::string, OrderPayLiquidate> orderPayLiquiMap;
	std::map<std::string, OrderPayLiquidate> orderRefundLiquiMap;
	std::map<std::string, OrderPayLiquidate> orderChannelLiquiMap;
	std::map<std::string, OrderPayLiquidate> orderChannelRefLiquiMap;

	NameValueMap m_InParams;
	NameValueMap m_RetMap;
	JsonMap m_ContentJsonMap;

	CMySQL m_mysql;

	ostringstream sqlss;

	clib_mysql * m_pBillDB = NULL;

};



#endif /* _CLIQUIDATIONTASK_H_ */
