#ifndef CDISTRIBUTION_DETAIL_H_
#define CDISTRIBUTION_DETAIL_H_

#include "IUrlProtocolTask.h"
#include "cJSON.h"
#include "mysqlapi.h"
#include "bill_protocol.h"
#include "json_util.h"
#include "log/clog.h"
#include "CSpeedPosConfig.h"
#include <map>


class CDistributionDetail: public IUrlProtocolTask
{
public:
    CDistributionDetail();
    virtual ~CDistributionDetail() { ; };
    INT32 Init()
    {
        m_bInited = true;
        return 0;
    }

    INT32 Execute(NameValueMap& mapInput, char** outbuf, int& outlen);

    void LogProcess() { };
    void Reset()
    {
        IUrlProtocolTask::Reset();
        m_stReq.Reset();
    }
protected:
    void clear();
    INT32 FillReq(NameValueMap& mapInput);
    void BuildResp(CHAR** outbuf, INT32& outlen);
    void getDetailBill();
    
private:
    stDistriBillP m_stReq;
    CMySQL m_mysql;
    clib_mysql* m_pBillDb;
    JsonMap m_jsonRsp;
};


#endif
