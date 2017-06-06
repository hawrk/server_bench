/*
 * TenpayClient.h
 *
 *  Created on: 2012-4-16
 *      Author: sangechen
 */

#ifndef TEN_MCH_PAYCLIENT_H_
#define TEN_MCH_PAYCLIENT_H_


#include "../../Base/Comm/types.h"
#include <sys/time.h>
#include "CObject.h"

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include "../../Base/Comm/business/order_protocol.h"
#include "../../Base/Comm/CIdGenClient.h"
#include "log/clog.h"
#include <string>


class CIdGenClient;

class TenMchpayClient 
{
public:
	TenMchpayClient(){}
	virtual ~TenMchpayClient(){}
    static const size_t SPID_LEN = 10;
    static const size_t ORDERID_LEN = 28;

	static int getOrderId(const string& sSupplierId, UIN_TYPE iUin, struct timeval& tv_now, string& sOrderId);

	static int getOrderIdEx(CIdGenClient& idGenClient, const string& sSupplierId, UIN_TYPE iUin, time_t& iCreateTime, string& sOrderId);

	static int getMerchantIdEx(CIdGenClient& idGenClient, const string& szPerfix, int iChannelId, int iMerIdLen, string& strMerchantId);

	static int getOrderIdEx_Mch(CIdGenClient& idGenClient, int iMchId, time_t& iCreateTime, string& sOrderId);

	static int ProcOrderNo(CIdGenClient& idGenClient, string& strMchId, std::string& strOrderNo);

    //财付通订单号：%10s+strftime(%Y%m%d%H%M%S)+%2d+%2d
    static int getTransactionId(uint32_t iPartnerId, UIN_TYPE iUin,
                                struct timeval& tv_now, string& sOrderId);

    static const size_t REFUNDID_LEN = 30;//财付通规格说明是28位，为了兼容历史数据，这里还是使用30
    //财付通退款单号：'109'+%10s+strftime(%Y%m%d%H%M%S)+%1d => %3d
    static int getRefundId(const string& sSupplierId,
                           struct timeval& tv_now, string& sRefundId);
    
	static int getOrderDbIndex(string& sOrderId);
    //to be implemented
    //int queryPayState();
    //int refund();
private:
    static sem_t* _sem;
};

#endif /* TENPAYCLIENT_H_ */
