#include "CDistributionSummary.h"
// cmd=6020&ver=1&type=2&beg_date=20170601&end_date=20170629&bm_id=1&pay_channel=WXPAY&mch_id=100000&mch_name=wx&shared_type=ch
CDistributionSummary::CDistributionSummary()
{
    Init();
}

INT32 CDistributionSummary::Execute(NameValueMap& mapInput, char** outbuf, int& outlen)
{
    CINFO_LOG("CDistributionSummary begin...");
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
    m_jsonRsp[JsonType("error")] = JsonType("0");
    m_jsonRsp[JsonType("msg")] = JsonType("");
    getSummary();
    BuildResp(outbuf, outlen);
    return 0;
}

INT32 CDistributionSummary::FillReq(NameValueMap& mapInput)
{
    FETCH_STRING_STD_NOERR(mapInput, m_stReq.beg_date, "BEG_DATE");
    FETCH_STRING_STD_NOERR(mapInput, m_stReq.end_date, "END_DATE");
    FETCH_STRING_STD(mapInput, m_stReq.bm_id, "BM_ID", -1, "fetch bm_id key error");
    FETCH_STRING_STD_NOERR(mapInput, m_stReq.pay_channel, "PAY_CHANNEL");
    FETCH_STRING_STD_NOERR(mapInput, m_stReq.mch_id, "MCH_ID");
    FETCH_STRING_STD_NOERR(mapInput, m_stReq.mch_name, "MCH_NAME");
    FETCH_STRING_STD_NOERR(mapInput, m_stReq.shared_type, "SHARED_TYPE");
    FETCH_INT_VALUE(mapInput, m_stReq.type, "TYPE", -1, "fetch type key error");
    CINFO_LOG("type:%d,beg_date:%s,end_date:%s,bm_id:%s,pay_channel:%s,mch_id:%s,mch_name:%s,shared_type:%s", m_stReq.type, m_stReq.beg_date.c_str(), m_stReq.end_date.c_str(), 
         m_stReq.bm_id.c_str(), m_stReq.pay_channel.c_str(), m_stReq.mch_id.c_str(), m_stReq.mch_name.c_str(), m_stReq.shared_type.c_str());
    return 0;
}

void CDistributionSummary::clear()
{
    m_stReq.Reset();
    m_pBillDb = NULL;
    m_jsonRsp.clear();
}

void CDistributionSummary::BuildResp(CHAR** outbuf, INT32& outlen)
{
    CHAR szResp[512];
    std::string resContent = JsonUtil::objectToString(m_jsonRsp);
    snprintf(szResp, sizeof(szResp), "%s\r\n", resContent.c_str());
    outlen = strlen(szResp);
    *outbuf = (CHAR*)malloc(outlen);
    memcpy(*outbuf, szResp, outlen);
    CDEBUG_LOG("Rsp :[%s]",szResp);
}

void CDistributionSummary::getSummary()
{
    ostringstream sqlss;
    int ret = 0;
    SqlResultSet outmap;
    std::map<string, string> mdata;
    sqlss.str("");

    if (m_stReq.type == 1)
    {
        // 商户清分
        sqlss << "SELECT SUM(trade_count) AS trade_count, SUM(trade_amount) AS trade_amount, SUM(refund_count) AS refund_count, SUM(refund_amount) AS refund_amount, SUM(cost_fee) AS cost_fee, SUM(mch_fee) AS mch_fee, SUM(profit) AS trade_profit ,SUM(unsettle) AS unsettle\
                FROM bill_db.t_bill_distribution WHERE fund_type = 'mch' AND bm_id='"
              << m_stReq.bm_id <<"'"
              << " AND bill_date >= '"
              << m_stReq.beg_date <<"'"
              << " AND bill_date <= '"
              << m_stReq.end_date <<"'";
        if (m_stReq.pay_channel != "")
        {
            sqlss << " AND pay_channel = '" <<m_stReq.pay_channel <<"'";
        }
        if (m_stReq.mch_id != "")
        {
            sqlss<< " AND mch_id = '" <<m_stReq.mch_id <<"'";
        }
        if (m_stReq.mch_name != "")
        {
            sqlss <<" AND mch_name LIKE '%" <<m_stReq.mch_name <<"%'";
        }
    }
    else if (m_stReq.type == 2)
    {
        // 机构清分
        sqlss <<"SELECT SUM(trade_count) AS trade_count, SUM(profit) AS profit , SUM(share_profit) AS share_profit FROM bill_db.t_bill_distribution WHERE bm_id = '"
              << m_stReq.bm_id <<"'"
              << " AND bill_date >= '"
              << m_stReq.beg_date <<"'"
              << " AND bill_date <= '"
              << m_stReq.end_date <<"'";
        if (m_stReq.pay_channel != "")
        {
            sqlss << " AND pay_channel = '" <<m_stReq.pay_channel <<"'";
        }
        if (m_stReq.mch_id != "")
        {
            sqlss<< " AND mch_id = '" <<m_stReq.mch_id <<"'";
        }
        if (m_stReq.mch_name != "")
        {
            sqlss <<" AND mch_name LIKE '%" <<m_stReq.mch_name <<"%'";
        }
        if (m_stReq.shared_type == "")
        {
            sqlss <<" AND fund_type = 'ch'" ;
        }
        else
        {
            sqlss <<" AND fund_type = '" <<m_stReq.shared_type <<"'";
        }
    }
    
    SQL_TRY_BEG();
    ret = m_mysql.QryAndFetchResMap(*m_pBillDb, sqlss.str().c_str(), outmap);
    SQL_TRY_END();
    if (ret == 1)
    {
        for (map<string, string>::iterator iter = outmap.begin(); iter != outmap.end(); ++iter)
        {
            CDEBUG_LOG("key:%s, value:%s", (iter->first).c_str(), (iter->second).c_str());
            m_jsonRsp[JsonType(iter->first)] = JsonType(iter->second);
        }
    }
    else
    {
        CINFO_LOG("no data found");
    }
}
