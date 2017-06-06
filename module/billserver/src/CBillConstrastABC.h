/*
 * CBillContrastABC.h
 *
 *  Created on: 2017年6月2日
 *      Author: hawrkchen
 */

#ifndef _CBILLCONTRASTABC_H_
#define _CBILLCONTRASTABC_H_

#include "CBillConstrastBase.h"


class CBillContrastABC : public CBillContrastBase
{
public:
	CBillContrastABC();
	virtual ~CBillContrastABC();

	virtual void BillFileDownLoad(ProPullBillReq& m_Req,int starttime);

	virtual void  LoadBillFlowToDB(ProPullBillReq& m_stReq,int starttime);

	virtual void GetRemitBillData(ProPullBillReq& m_stReq,int starttime,int endtime);

	virtual void ProcBillComparison(ProPullBillReq& m_stReq,int starttime,int endtime);

	virtual void UpdateBillStatus(ProPullBillReq& m_stReq,int starttime);

	/*
	 * AddSettleLog 入参不一样，不是继续自基类
	 */
	virtual void AddSettleLog(TRemitBill& remitBill,std::map<int, TRemitBill>& tremitMap,int& iIndex,int& total_amount);


	virtual INT32 NotifySettleServer(ProPullBillReq& m_stReq);

protected:
	CSyBase* pBillDb;
	CPayTransSybaseDao m_stTransFlowDao;
};



#endif /* _CBILLCONTRASTABC_H_ */
