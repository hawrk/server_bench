/*
 * CAgentPayBillDealTask.h
 * 
 *  Created on: 2010-5-20
 *      Author: 
 */

#ifndef _C_APAY_BILL_DEAL_TASK_H_
#define _C_APAY_BILL_DEAL_TASK_H_

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


class CAgentPayBillDealTask : public IUrlProtocolTask
{
public:
	CAgentPayBillDealTask(){}
	virtual ~CAgentPayBillDealTask(){}

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
		
		m_BillFlag = true;

		m_InParams.clear();
		m_RetMap.clear();

		pBillBusConfig = NULL;

		m_FeeCnt = 0;
		m_NumCnt = 0;

		m_RefFeeCnt = 0;
		m_RefNumCnt = 0;
    }

protected:
    void FillReq( NameValueMap& mapInput);
    void CheckInput();
    void BuildResp( CHAR** outbuf, INT32& outlen );
	void SetRetParam();

	//业务处理
	void Deal();

	//获取账单，并入库
	void GetAndLoadBillFile();

	//对账操作
	void CompareDeal();

	//对成功单
	void CompareSucce();

	//对退票单
	void CompareRefund();

	//查询订单
	void QryOrder(NameValueMap& inMap,SqlResultSet &outMap);

	//更新订单状态
	void UpdateOrder(SqlResultSet& inMap,string szStatus);

	//异常账处理
	void AbnormalOrderDeal(SqlResultSet& inMap,string szErrMsg);

	//生成商户对账文件
	void CreateMchBillFile();

	//处理跨天交易的对账单
	void DealAcrossDayBill(SqlResultSet & sqlSet);

	//查询对账日志表
	void QryBillLog();

	//更新对账状态
	void UpdateBillLog(bool bFlag,string msg);

    /** 用来上报统计 */
    struct timeval m_stStart;
    struct timeval m_stEnd;
	
    int m_istart;
    int m_iend;
	int m_iBillBeginTime;
	int m_iBillEndTime;

	long long  m_FeeCnt;
	long m_NumCnt;

	long long  m_RefFeeCnt;
	long m_RefNumCnt;


	bool m_BillFlag;
	
	CMySQL m_SqlHandle;

	NameValueMap m_InParams;
	NameValueMap m_RetMap;

	CBillBusiConfig* pBillBusConfig;
};

#endif /* CCREATEORDER_TASK_H_ */

