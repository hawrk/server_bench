/*
 * CBillCheckTask.h
 *
 *  Created on: 2017年7月19日
 *      Author: hawrkche
 *      Desc:对账操作
 */

#ifndef _CBILLCHECKTASK_H_
#define _CBILLCHECKTASK_H_

#include "IUrlProtocolTask.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../Base/Comm/clib_mysql.h"
#include "../Base/common/comm_tools.h"
#include "CheckParameters.h"
#include "CRouteBillBase.h"
#include "util/tc_file.h"

class CBillCheckTask : public IUrlProtocolTask
{
public:
	CBillCheckTask();
	virtual ~CBillCheckTask(){}

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

		orderPayBillMap.clear();
		orderRefundBillMap.clear();
		bankPayBillMap.clear();
		bankRefundBillMap.clear();
		m_pBillDB = Singleton<CSpeedPosConfig>::GetInstance()->GetBaseDB();
    }

protected:
    void FillReq( NameValueMap& mapInput);
    void CheckInput();
    void BuildResp( CHAR** outbuf, INT32& outlen );

    INT32 CalcEffectiveTimeBill();

    void CheckBillTask();

    void LoadBillFiletoDB();

    void CallPayGate2GetFactorID();

    void GetBillData();

    void CompareSuccess();

    void CompareRefund();

    void InsertAbnormalDB(const string& bill_type,int ab_type,
    		const string& order_no = "", const string& order_amt = "",const string& bank_no = "", const string& bank_amt = "");

    void UpdateCheckBillDB(const string& order_no);

    void InsertIntoSummary();

    INT32 BankOverflowQuery(string& order_no,int order_flag);


    void TracateBankDB(const string& channel);


    void SendCreatBillRequest();

	void SetRetParam();

	std::string m_start_time;
	std::string m_end_time;

	NameValueMap m_InParams;
	NameValueMap m_RetMap;
	JsonMap m_ContentJsonMap;

	vector<string> factor_vec;


	int m_abnor_num;  //异常笔数
	long m_abnor_amount;   //异常金额

	//对账Map
	std::map<std::string, OrderPayBillSummary> orderPayBillMap;
	std::map<std::string, OrderRefundBillSummary> orderRefundBillMap;
	std::map<std::string, BankpayBillSummary> bankPayBillMap;
	std::map<std::string, BankpayBillSummary> bankRefundBillMap;


	ostringstream sqlss;
	CMySQL m_mysql;
	clib_mysql * m_pBillDB = NULL;

};



#endif /* _CBILLCHECKTASK_H_ */
