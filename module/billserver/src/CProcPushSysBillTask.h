/*
 * CCreateOrderTask.h
 * 创建订单接口，生成团购凭证，返回订单号
 *  Created on: 2010-5-20
 *      Author: rogeryang
 */

#ifndef _C_PROC_PUSH_SYS_BILL_TASK_H
#define _C_PROC_PUSH_SYS_BILL_TASK_H

#include "IUrlProtocolTask.h"
#include "CSpeedPosConfig.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "CSpeedPosServer.h"
#include "util/tc_parsepara.h"
#include "util/tc_file.h"

class CProcPushSysBillTask : public IUrlProtocolTask
{
public:
	CProcPushSysBillTask(){}
	virtual ~CProcPushSysBillTask(){Reset();}

    INT32 Init()
    {
        m_bInited = true;
        return 0;
    }

	INT32 createSubProcess(clib_mysql* pDbCon);

	INT32 checkSubProcess();

	INT32 CalcEffectiveTimeBill();

	INT32 CheckBillFill();

	INT32 HandleProcess();

    INT32 Execute( NameValueMap& mapInput, char** outbuf, int& outlen );

    /*
     * @brief 生成系统对账单入口
     * @param
     * @return int
     */
    INT32 PushSysPaymentBill(clib_mysql* pDbCon,const int& iDbIndex,bool bFirstTab);

    /*
     * @brief 生成系统对账单
     */
    INT32 CreateSysPaymentBillFill(clib_mysql* pDbCon,
    										std::string strOrderChannelTableName,
    										std::vector<OrderFlowData>& vecOrderFlowDatas);
    /*
     * @brief 生成退款单入口
     */
    INT32 PushSysRefundBill();

    /*
     * @brief 微信申请下载微信对账单
     */
    INT32 PushWxBill();

    /*
     * @brief 支付宝申请下载支付宝对账单
     */
    int PushAliBill(std::string& strPath,std::string& detailsuffix,std::string& strGatewayUrl, std::string& strToName);


    void LogProcess();
    void Reset()
    {
        IUrlProtocolTask::Reset();
		m_iBillBeginTime = 0;
		m_iBillEndTime = 0;
		m_stResp.Reset();
		sMchBillSrcFillName = "";
		sMchBillEncFileName = "";
		sChannelBillSrcFillName = "";
		sChannelBillEncFileName = "";
		if (pBillShopFile) { delete pBillShopFile; pBillShopFile = NULL; }
		if (pBillChannelFile) { delete pBillChannelFile; pBillChannelFile = NULL; }

		sWxBillSrcFillName = "";
		sWxBillEncFileName = "";
		if (pBillWxFile) { delete pBillWxFile; pBillWxFile = NULL; }

		sAliBillSrcFillName = "";
		sAliBillEncFileName = "";
    }

protected:
    INT32 FillReq( NameValueMap& mapInput);
    INT32 CheckInput();
    void BuildResp( CHAR** outbuf, INT32& outlen );

	ProPullBillReq m_stReq;
	BillServerResponse m_stResp;
	CBillFile* pBillShopFile;
	CBillFile* pBillChannelFile;
	vector<pid_t> vecPid;
	
	CPayTransactionFlowDao m_stTransFlowDao;
	CSpeedPosServer m_speedposserver;

	int m_iBillBeginTime;
	int m_iBillEndTime;

	ErrParamMap errMap;

	std::string sMchBillSrcFillName;
	std::string sMchBillEncFileName;
	std::string sChannelBillSrcFillName;
	std::string sChannelBillEncFileName;

	std::string sWxBillSrcFillName;
	std::string sWxBillEncFileName;
	CBillFile* pBillWxFile;

	std::string sAliBillSrcFillName;
	std::string sAliBillEncFileName;

};

#endif /* CCREATEORDER_TASK_H_ */
