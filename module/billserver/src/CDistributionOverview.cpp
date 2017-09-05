#include "CDistributionOverview.h"
// cmd=6010&beg_date=20170601&end_date=20170621&ver=1&bm_id=1
CDistributionOverview::CDistributionOverview()
{
    Init();
}

INT32 CDistributionOverview::Execute(NameValueMap& mapInput, char** outbuf, int& outlen)
{
    CINFO_LOG("CDistributionOverview begin...");
    if (!m_bInited)
    {
        snprintf( m_szErrMsg, sizeof(m_szErrMsg), "Not Inited" );
        m_iRetCode = -1;
        return m_iRetCode;
    }
    clear();
    m_pBillDb = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
    int ret = FillReq(mapInput);
    if (ret < 0)
    {
        CERROR_LOG("input format error,ret:%d", ret);
        return 0;
    }
    m_jsonRsp.insert(JsonMap::value_type(JsonType("error"), JsonType("0")));
    m_jsonRsp.insert(JsonMap::value_type(JsonType("msg"), JsonType("")));
    getChanAmount();
    getDetailFee();
    BuildResp(outbuf, outlen);
    return 0;
}

INT32 CDistributionOverview::FillReq(NameValueMap& mapInput)
{
    FETCH_STRING_VALUE(mapInput, m_stReq.beg_date, "BEG_DATE", -1, "beg_date parse error");
    FETCH_STRING_VALUE(mapInput, m_stReq.end_date, "END_DATE", -2, "end_date parse error");
    FETCH_STRING_VALUE(mapInput, m_stReq.bm_id, "BM_ID", -3, "bm_id parse error");
    CINFO_LOG("beg_date:%s, end_date:%s, bm_id:%s", m_stReq.beg_date, m_stReq.end_date, m_stReq.bm_id);
    return 0;
}

void CDistributionOverview::getChanAmount()
{
    char sql[512] = {};
    int ret = 0;
    SqlResultMapVector outmap;

    sprintf(sql, "SELECT pay_channel, SUM(profit) -sum(cost_fee) as total FROM bill_db.`t_bill_distribution` "
    		"WHERE bill_date >= '%s' AND bill_date <= '%s' AND bm_id = '%s' and fund_type ='mch' GROUP BY pay_channel",
        m_stReq.beg_date, m_stReq.end_date, m_stReq.bm_id);
    SQL_TRY_BEG();
    ret = m_mysql.QryAndFetchResMVector(*m_pBillDb, sql, outmap);
    SQL_TRY_END();
    std::map<string, string> mdata;
    mdata["wx_amount"] = "0";
    mdata["zfb_amount"] = "0";
    if (ret == 1)
    {

        for (unsigned int i = 0; i < outmap.size(); ++i)
        {
            if (outmap[i]["pay_channel"] == "WXPAY")
            {
                mdata["wx_amount"] = outmap[i]["total"];
            }
            else if (outmap[i]["pay_channel"] == "ALIPAY")
            {
                mdata["zfb_amount"] = outmap[i]["total"];
            }
            else
            {
                CINFO_LOG("unkown pay_channel:%s", outmap[i]["pay_channel"].c_str());
            }
        }
    }
    else
    {
        CINFO_LOG("no ChanAmount data found from bill_db");
    }
    for (map<string, string>::iterator iter = mdata.begin(); iter != mdata.end(); ++iter)
    {
        m_jsonRsp.insert(JsonMap::value_type(JsonType(iter->first), JsonType(iter->second)));
    }
    
}

void CDistributionOverview::getDetailFee()
{
    char sql[512] = {};
    int ret = 0;
    SqlResultMapVector outmap;

    sprintf(sql, "SELECT fund_type, SUM(settle_amt) AS amount FROM bill_db.t_bill_settle WHERE bill_date >= '%s' AND bill_date <= '%s' AND bm_id = '%s' GROUP BY fund_type",
        m_stReq.beg_date, m_stReq.end_date, m_stReq.bm_id);
    SQL_TRY_BEG();
    ret = m_mysql.QryAndFetchResMVector(*m_pBillDb, sql, outmap);
    SQL_TRY_END();
    std::map<string, string> mdata;
    mdata["mch_amount"] = "0";
    mdata["channel_amount"] = "0";
    mdata["service_amount"] = "0";
    mdata["plat_amount"] = "0";
    if (ret == 1)
    {
        for (unsigned int i = 0; i < outmap.size(); ++i)
        {
            if (outmap[i]["fund_type"] == "bm")
            {
                mdata["plat_amount"] = outmap[i]["amount"];
            }
            else if (outmap[i]["fund_type"] == "serv")
            {
                mdata["service_amount"] = outmap[i]["amount"];
            }
            else if (outmap[i]["fund_type"] == "ch")
            {
                mdata["channel_amount"] = outmap[i]["amount"];
            }
            else if (outmap[i]["fund_type"] == "mch")
            {
                mdata["mch_amount"] = outmap[i]["amount"];
            }
            else
            {
                CINFO_LOG("unkown fund type:%s", outmap[i]["fund_type"].c_str());
            }
        }
    }
    else
    {
        CINFO_LOG("no DetailFee data found from bill_db");
    }
    for (map<string, string>::iterator iter = mdata.begin(); iter != mdata.end(); ++iter)
    {
        m_jsonRsp.insert(JsonMap::value_type(JsonType(iter->first), JsonType(iter->second)));
    }
}

void CDistributionOverview::clear()
{
    m_stReq.Reset();
    m_pBillDb = NULL;
    m_jsonRsp.clear();
}

void CDistributionOverview::BuildResp(CHAR** outbuf, INT32& outlen)
{
    CHAR szResp[1024];
    std::string resContent = JsonUtil::objectToString(m_jsonRsp);
    snprintf(szResp, sizeof(szResp), "%s\r\n", resContent.c_str());
    outlen = strlen(szResp);
    *outbuf = (CHAR*)malloc(outlen);
    memcpy(*outbuf, szResp, outlen);
    CDEBUG_LOG("Rsp :[%s]",szResp);
}
