/*
 * TenMchpayClient.cpp
 *
 *  Created on: 2012-4-16
 *      Author: sangechen
 */

#include "TenMchpayClient.h"
#include "id_protocol.h"
#include "CIdGenClient.h"
#include "log/clog.h"
#include <stdlib.h> 



sem_t* TenMchpayClient::_sem = sem_open("/OrderServer_seq", O_CREAT|O_RDWR, 0777, 1);


int TenMchpayClient::getOrderId(const string& sSupplierId, UIN_TYPE iUin, struct timeval& tv_now, string& sOrderId)
{
    if (sSupplierId.length() != SPID_LEN
            || iUin < 10000)
    {
        return -1;
    }

    struct tm tm_local;
    char szOrderId[256]={0};
    size_t buf_len = 0;

    //订单号前10位是财付通商户号
    buf_len += snprintf(szOrderId, sizeof(szOrderId), "%10s", sSupplierId.c_str());

    //得到系统当前的精确时间
    gettimeofday(&tv_now, NULL);

    localtime_r(&tv_now.tv_sec, &tm_local);
    //追加20120416152035
    buf_len += strftime(szOrderId+buf_len, sizeof(szOrderId)-buf_len, "%Y%m%d%H%M%S", &tm_local);

    //追加microsecond(10E-6)的前两位, 0.XX sec
    buf_len += snprintf(szOrderId+buf_len, sizeof(szOrderId)-buf_len, "%02ld", tv_now.tv_usec / 10000);

    //追加iUin的后两位
    buf_len += snprintf(szOrderId+buf_len, sizeof(szOrderId)-buf_len, "%02lld", iUin % 100);

    if (buf_len == ORDERID_LEN)
    {
        sOrderId.assign(szOrderId, buf_len);
        return 0;
    }
    else
    {
        return -2;
    }
};

int TenMchpayClient::getOrderIdEx(CIdGenClient& idGenClient, const string& sSupplierId, UIN_TYPE iUin, time_t& iCreateTime, string& sOrderId)
{
    if (sSupplierId.length() != SPID_LEN
            || iUin < 10000)
    {
        return -1;
    }

    char szQQSuffix[16]={0};
    snprintf(szQQSuffix, sizeof(szQQSuffix) - 1, "%02lld", iUin % 100);

    string strTime_t;
    INT32 iRet = idGenClient.GetTimestampUniqStr(sOrderId, strTime_t, sSupplierId.substr(0, 10).c_str(), szQQSuffix);

    if(0 == iRet && ORDER_ID_LEN <= sOrderId.length() && 0 < strTime_t.length())
    {
        iCreateTime = (time_t)atoll(strTime_t.c_str());
        return 0;
    }
    else
    {
        return -2;
    }
};

/*
 * 获取商户ID
 */
int TenMchpayClient::getMerchantIdEx(CIdGenClient& idGenClient, const string& szPerfix, int iChannelId, int iMerIdLen, string& strMerchantId)
{
	INT32 iRet = 0;
	if (iChannelId < 0 || szPerfix.length() < 0)
	{
		return -1;
	}

	char szMerchantPerfix[16] = { 0 };
	snprintf(szMerchantPerfix, sizeof(szMerchantPerfix) - 1, "%02d", iChannelId % 100);
	
	int genCount = 1;
	int genMerIdLen = iMerIdLen - 2;

	string strTime_t;
	vector<string> vecIDs;
	iRet = idGenClient.GetID(ID_TYPE_RANDOM, szPerfix.c_str(), genMerIdLen, genCount, "", vecIDs);
	if (iRet != 0 || (int)vecIDs.size() != genCount)
	{
		return -1;
	}

	char szMerchantId[16] = { 0 };
	snprintf(szMerchantId, sizeof(szMerchantId), "%s%s", vecIDs[0].c_str(), szMerchantPerfix);
	strMerchantId.assign(szMerchantId);

    CDEBUG_LOG("TenMchpayClient::getMerchantIdEx.GetID. szPerfix:[%s] genMerIdLen:[%d] genCount[%d] Id[%s] szMerchantId[%s] strMerchantId[%s].",
		szPerfix.c_str(), genMerIdLen,
		genCount, vecIDs[0].c_str(), 
		szMerchantId, strMerchantId.c_str());

	return 0;
}


int TenMchpayClient::getOrderIdEx_Mch(CIdGenClient& idGenClient, int iMchId, time_t& iCreateTime, string& sOrderId)
{
	if (iMchId < 0)
	{
		return -1;
	}

	char szMchSuffix[16] = { 0 };
	snprintf(szMchSuffix, sizeof(szMchSuffix)-1, "%02d", iMchId % 100);

	char szMchId[16] = { 0 };
	snprintf(szMchId, sizeof(szMchId)-1, "%d", iMchId);
    std::string sMchId = szMchId;

	std::string strTime_t;
	int iRet = idGenClient.GetTimestampUniqStr(sOrderId, strTime_t, sMchId.substr(0, 10).c_str(), szMchSuffix);

    CDEBUG_LOG( "getorderid return, sOrderId:[%s] Ret:[%d] timelength:[%d].\n", sOrderId.c_str(),
    iRet, strTime_t.length() );
	if (0 == iRet && ORDER_ID_LEN <= sOrderId.length() && 0 < strTime_t.length())
	{
		iCreateTime = (time_t)atoll(strTime_t.c_str());
		return 0;
	}
	else
	{
		return -2;
	}
}

int TenMchpayClient::ProcOrderNo(CIdGenClient& idGenClient,string& strMchId, std::string& strOrderNo)
{
	if (strMchId.empty())
	{
		return -1;
	}
	int iRet = idGenClient.GetOrderNo(strMchId,strOrderNo);
	if (0 != iRet)
	{
		return iRet;
	}
	return 0;
}

/*
 * 生成财付通支付订单号，格式如下：
 * %10s+strftime(%Y%m%d%H%M%S)+%2d+%2d
 *
 * 整个生成算法是以时间为依据，重复的概率取决于系统的并发量.
 * 出现冲突后有两种处理方式：
 * 1.重新调用该函数获取新订单号
 * 2.直接返回出错(在0.01秒内，位数为XY的QQ，对于一个商户号只能创建一个订单)
 *
 */
int TenMchpayClient::getTransactionId(uint32_t iPartnerId, UIN_TYPE iUin,
                                   struct timeval& tv_now, string& sOrderId)
{
    if (iPartnerId < 1000000000L //商户号是10位正整数
     || iUin < 10000)
    {
        return -1;
    }

    struct tm tm_local;
    char szOrderId[256]={0};
    size_t buf_len = 0;

    //订单号前10位是财付通商户号
    buf_len += snprintf(szOrderId, sizeof(szOrderId), "%u", iPartnerId);

    //得到系统当前的精确时间
    gettimeofday(&tv_now, NULL);

    localtime_r(&tv_now.tv_sec, &tm_local);
    //追加20120416152035
    buf_len += strftime(szOrderId+buf_len, sizeof(szOrderId)-buf_len, "%Y%m%d%H%M%S", &tm_local);

    //追加microsecond(10E-6)的前两位, 0.XX sec
    buf_len += snprintf(szOrderId+buf_len, sizeof(szOrderId)-buf_len, "%02ld", tv_now.tv_usec / 10000);

    //追加iUin的后两位
    buf_len += snprintf(szOrderId+buf_len, sizeof(szOrderId)-buf_len, "%02lld", iUin % 100); //UIN_FMT

    if (buf_len == ORDERID_LEN)
    {
        sOrderId.assign(szOrderId, buf_len);
        return 0;
    }
    else
    {
        return -2;
    }
}

int TenMchpayClient::getRefundId(const string& sSupplierId,
                              struct timeval& tv_now, string& sRefundId)
{
    if (sSupplierId.length() != SPID_LEN)
    {
        return -1;
    }

    struct tm tm_local;
    char szRefundId[256]={0};
    size_t buf_len = 0;

    //订单号前10位是财付通商户号
    buf_len += snprintf(szRefundId, sizeof(szRefundId), "109%10s", sSupplierId.c_str());

    //得到系统当前的精确时间
    gettimeofday(&tv_now, NULL);

    localtime_r(&tv_now.tv_sec, &tm_local);
    //追加20120416152035
    buf_len += strftime(szRefundId+buf_len, sizeof(szRefundId)-buf_len, "%Y%m%d%H%M%S", &tm_local);

    int sem_val=0;
    sem_post(_sem);
    sem_getvalue(_sem, &sem_val);

    //追加microsecond(10E-6)的第1位, 0.X sec
    buf_len += snprintf(szRefundId+buf_len, sizeof(szRefundId)-buf_len, "%03d", sem_val);//tv_now.tv_usec/1000);

    if (buf_len == REFUNDID_LEN)
    {
        sRefundId.assign(szRefundId, buf_len);
        return 0;
    }
    else
    {
        return -2;
    }
}

int TenMchpayClient::getOrderDbIndex(string& sOrderId)
{
   char szOrderId[256] = { 0 };
   time_t rawtime;
   struct tm* pTmNow;

   rawtime = time(NULL);

   pTmNow = localtime(&rawtime);
								 
   strftime(szOrderId, sizeof(szOrderId), "%Y%m", pTmNow);
   
   sOrderId = szOrderId;

   return 0;
}

/*int main(int argc, char **argv) {
    const char* spid="1234567890";
    UIN_TYPE iUin=1000000;

    struct timeval tv_now;
    string sId;
    int iRet = TenMchpayClient::getTransactionId(spid,iUin,tv_now,sId);
    printf("%s,%ld\n%d~%s\n", ctime(&tv_now.tv_sec), tv_now.tv_usec, iRet, sId.c_str());

    iRet = TenMchpayClient::getRefundId(spid,tv_now,sId);
    printf("%s,%ld\n%d~%s\n", ctime(&tv_now.tv_sec), tv_now.tv_usec, iRet, sId.c_str());

}*/

							 
