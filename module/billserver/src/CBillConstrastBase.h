/*
 * CBillContrastBase.h
 *
 *  Created on: 2017年6月2日
 *      Author: hawrkchen
 */

#ifndef _CBILLCONTRASTBASE_H_
#define _CBILLCONTRASTBASE_H_


#include "IUrlProtocolTask.h"
#include <stdlib.h>
#include "CBillFile.h"
#include "log/clog.h"
#include "CSpeedPosConfig.h"
#include "util/tc_file.h"
#include "network.h"

class CBillContrastBase
{
public:
	CBillContrastBase();

	virtual ~CBillContrastBase();

	virtual void CheckBillFill(ProPullBillReq& m_Req,int starttime);

    /*
     * @brief 下载对账单
     */
	virtual void  BillFileDownLoad(ProPullBillReq& m_Req,int starttime);

	/*
	 * @brief 对账单文件导入到DB中
	 * 默认为Mysql,如果是sybase需要重新定义
	 */
	virtual void  LoadBillFlowToDB(ProPullBillReq& m_stReq,int starttime);

	/*
	 * @brief 取得对账表中数据放到内存中
	 */
	virtual void GetRemitBillData(ProPullBillReq& m_Req,int starttime,int endtime);

	/*
	 * @brief 对账操作入口
	 */
	virtual void ProcBillComparison(ProPullBillReq& m_stReq,int starttime,int endtime);

	/*
	 * @brief 结算操作
	 */
	virtual void ProRemitBillProcess(ProPullBillReq& m_Req,int starttime);

	/*
	 * @brief 更新对账和结算状态
	 */
	virtual void UpdateBillStatus(ProPullBillReq& m_stReq,int starttime);

	/*
	 * @brief 写入结算记录
	 */
	virtual void AddSettleLog(ProPullBillReq& m_Req,TRemitBill& remitBill,std::map<int, TRemitBill>& tremitMap,int& iIndex,int& total_amount);


	virtual INT32 NotifySettleServer(ProPullBillReq& m_stReq);


	/*
	 * @brief 使用SFTP方式下载
	 */
	void SFTPDownLoad(ProPullBillReq& m_Req,int starttime);

	/*
	 * @brief 直接复制
	 */
	void Copy2GetFile(ProPullBillReq& m_Req,int starttime);

protected:

	CBillBusiConfig* pBillBusConfig;

	CBillFile* pBillWxFile;
	CBillFile* pBillAliFile;
	CBillFile* pBillShopFille;
	CBillFile* pBillChannelFille;
	CBillFile* pSettleFile;
	CBillFile* pSettleCipherFile;

	int m_total_count;   //结算总笔数
	int m_total_amount;  //结算总金额

	std::string sShopBillSrcFileName;
	std::string sShopBillDecFileName;
	std::string sChannelBillSrcFileName;
	std::string sChannelBillDecFileName;

	std::string sWxBillSrcFileName;
	std::string sWxBillDecFileName;
	std::string sAliBillSrcFileName;
	std::string sAliBillDecFileName;


	ErrParamMap errMap;
	StepMap    stepMap;

	std::map<int, TRemitBill> tremitMap;
	std::map<std::string, OrderPayBillSumary> orderPayBillMap;
	std::map<std::string, OrderRefundBillSumary> orderRefundBillMap;
	std::map<std::string, int> channelPayMap;
	std::map<std::string, int> channelRefundMap;

};



#endif /* _CBILLCONTRASTBASE_H_ */
