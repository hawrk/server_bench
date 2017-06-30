/*
 * CBillConstrastSPDB.h
 *
 *  Created on: 2017年6月8日
 *      Author: hawrkchen
 *      Desc :for 浦发银行
 */

#ifndef _CBILLCONSTRASTSPBD_H_
#define _CBILLCONSTRASTSPBD_H_


#include "CBillConstrastBase.h"
#include "apayErrorNo.h"

class CBillContrastSPDB : public CBillContrastBase
{
public:
	CBillContrastSPDB();
	virtual ~CBillContrastSPDB();

	virtual void InitBillFileDownLoad(ProPullBillReq& m_Req,int starttime);

	virtual void BillFileDownLoad(ProPullBillReq& m_stReq,int starttime);

	virtual void  LoadBillFlowToDB(ProPullBillReq& m_stReq,int starttime);

	virtual void GetRemitBillData(ProPullBillReq& m_stReq,int starttime,int endtime);

	virtual void ProcBillComparison(ProPullBillReq& m_stReq,int starttime,int endtime);

	virtual void UpdateBillStatus(ProPullBillReq& m_stReq,int starttime);

	virtual void ProRemitBillProcess(ProPullBillReq& m_Req,int starttime);

	virtual void ProcException(ProPullBillReq& m_stReq,int starttime,int exce_type = 0);

	void AddSettleLog(ProPullBillReq& m_Req,TRemitBill& remitBill,std::map<int, TRemitBill>& tremitMap,int& iIndex,int& total_amount);

	std::string GetPayChannelCode(ProPullBillReq& m_stReq);

	void UpdateDownLoadDB(ProPullBillReq& m_stReq,int bill_status,const char* file_name);

	void UpdateBillBatchDB(ProPullBillReq& m_stReq);

	void InitBillBatchDB(ProPullBillReq& m_stReq);

	void GetBatchNo(const string& strtype,const string& bill_date,string& seq_no);

	//处理订单溢出
	void ProcOverFlowData(const string& resBody,const string& tableName);

protected:
	clib_mysql* pBillDb;
	CPayTransactionFlowDao m_stTransFlowDao;

	std::string m_batch_no;

};


#endif /* _CBILLCONSTRASTSPBD_H_ */
