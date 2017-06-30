/*
 * CBillContrastCommon.h
 *
 *  Created on: 2017年6月2日
 *      Author: hawrkchen
 */

#ifndef _CBILLCONTRASTCOMMON_H_
#define _CBILLCONTRASTCOMMON_H_

#include "CBillConstrastBase.h"
#include "apayErrorNo.h"

class CBillContrastCommon : public CBillContrastBase
{
public:
	CBillContrastCommon();
	virtual ~CBillContrastCommon();

	virtual void BillFileDownLoad(ProPullBillReq& m_stReq,int starttime);

	virtual void  LoadBillFlowToDB(ProPullBillReq& m_stReq,int starttime);

	virtual void GetRemitBillData(ProPullBillReq& m_stReq,int starttime,int endtime);

	virtual void ProcBillComparison(ProPullBillReq& m_stReq,int starttime,int endtime);

	virtual void UpdateBillStatus(ProPullBillReq& m_stReq,int starttime);

	virtual void InitBillFileDownLoad(ProPullBillReq& m_Req,int starttime);

	virtual void ProcException(ProPullBillReq& m_stReq,int starttime,int exce_type = 0);

protected:
	clib_mysql* pBillDb;
	CPayTransactionFlowDao m_stTransFlowDao;
};



#endif /* _CBILLCONTRASTCOMMON_H_ */
