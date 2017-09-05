#include "CDistributionDetail.h"
// cmd // cmd=6030&ver=1&type=1&beg_date=20170601&end_date=20170629&bm_id=1&pay_channel=WXPAY&mch_id=100000&mch_name=wx&shared_type=ch&page=1&page_count=10
CDistributionDetail::CDistributionDetail()
{
    Init();
}

INT32 CDistributionDetail::Execute(NameValueMap& mapInput, char** outbuf, int& outlen)
{
    CINFO_LOG("CDistributionDetail begin...");
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
    m_jsonRsp[JsonType("count")] = JsonType("0");
    getDetailBill();
    BuildResp(outbuf, outlen);
    return 0;
}

INT32 CDistributionDetail::FillReq(NameValueMap& mapInput)
{
    FETCH_STRING_STD_NOERR(mapInput, m_stReq.beg_date, "BEG_DATE");
    FETCH_STRING_STD_NOERR(mapInput, m_stReq.end_date, "END_DATE");
    FETCH_STRING_STD(mapInput, m_stReq.bm_id, "BM_ID", -1, "fetch bm_id key error");
    FETCH_STRING_STD_NOERR(mapInput, m_stReq.pay_channel, "PAY_CHANNEL");
    FETCH_STRING_STD_NOERR(mapInput, m_stReq.mch_id, "MCH_ID");
    FETCH_STRING_STD_NOERR(mapInput, m_stReq.mch_name, "MCH_NAME");
    FETCH_STRING_STD_NOERR(mapInput, m_stReq.shared_type, "SHARED_TYPE");
    FETCH_INT_VALUE(mapInput, m_stReq.type, "TYPE", -2, "fetch type key error");
    FETCH_INT_VALUE(mapInput, m_stReq.page, "PAGE", -3, "fetch page key error");
    FETCH_INT_VALUE(mapInput, m_stReq.page_count, "PAGE_COUNT", -4, "fetch page_count key error");
    CINFO_LOG("type:%d,beg_date:%s,end_date:%s,bm_id:%s,pay_channel:%s,mch_id:%s,mch_name:%s,shared_type:%s,page:%d,page_count:%d", m_stReq.type, m_stReq.beg_date.c_str(), m_stReq.end_date.c_str(), 
         m_stReq.bm_id.c_str(), m_stReq.pay_channel.c_str(), m_stReq.mch_id.c_str(), m_stReq.mch_name.c_str(), m_stReq.shared_type.c_str(), m_stReq.page, m_stReq.page_count);
    return 0;
}

void CDistributionDetail::clear()
{
    m_stReq.Reset();
    m_pBillDb = NULL;
    m_jsonRsp.clear();
}

void CDistributionDetail::BuildResp(CHAR** outbuf, INT32& outlen)
{
    std::string resContent = JsonUtil::objectToString(m_jsonRsp);
    outlen = resContent.size()+3;
    *outbuf = (CHAR*)malloc(outlen);
    snprintf(*outbuf, outlen, "%s\r\n", resContent.c_str());
    CDEBUG_LOG("Rsp :[%s]",*outbuf);
}

void CDistributionDetail::getDetailBill()
{
    ostringstream sqlss;
    ostringstream count_sql;

    int ret = 0;
    SqlResultMapVector outmap;
    SqlResultSet cm;
    std::map<string, string> mdata;
    sqlss.str("");
    count_sql.str("");
    if (m_stReq.type == 1)
    {
        // 商户清分详情
        count_sql <<"SELECT count(*) as ct FROM bill_db.t_bill_distribution "
                  <<"  WHERE bm_id = '" <<m_stReq.bm_id
                  <<"' AND bill_date >= '" <<m_stReq.beg_date
                  <<"' AND bill_date <= '" <<m_stReq.end_date <<"' AND fund_type = 'mch'";

        sqlss <<"SELECT mch_id, mch_name, bill_date, pay_channel, org_id, trade_count, trade_amount, refund_count, refund_amount, cost_fee, mch_fee, profit, settle_cycle, unsettle FROM bill_db.t_bill_distribution "
              <<"  WHERE bm_id = '" <<m_stReq.bm_id
              <<"' AND bill_date >= '" <<m_stReq.beg_date
              <<"' AND bill_date <= '" <<m_stReq.end_date <<"'";

        if (m_stReq.pay_channel != "")
        {
            count_sql <<" AND pay_channel = '" <<m_stReq.pay_channel <<"'";
            sqlss <<" AND pay_channel = '" <<m_stReq.pay_channel <<"'";
        }
        if (m_stReq.mch_id != "")
        {
            count_sql <<" AND mch_id = '" <<m_stReq.mch_id <<"'";
            sqlss <<" AND mch_id = '" <<m_stReq.mch_id <<"'";
        }
        if (m_stReq.mch_name != "")
        {
            count_sql <<" AND mch_name LIKE '%" <<m_stReq.mch_name <<"%' ";
            sqlss <<" AND mch_name LIKE '%" <<m_stReq.mch_name <<"%' ";
        }
        sqlss <<" AND fund_type = 'mch' ORDER BY bill_date LIMIT " <<(m_stReq.page-1)*m_stReq.page_count <<"," <<m_stReq.page_count;
    }
    else if (m_stReq.type == 2)
    {
        count_sql <<"SELECT count(*) as ct FROM bill_db.t_bill_distribution "
                  <<"  WHERE bm_id = '" <<m_stReq.bm_id
                  <<"' AND bill_date >= '" <<m_stReq.beg_date
                  <<"' AND bill_date <= '" <<m_stReq.end_date <<"'";
        sqlss <<"SELECT bill_date, mch_id, mch_name, pay_channel, trade_count, profit, fund_type, share_profit FROM bill_db.t_bill_distribution "
              <<"  WHERE bm_id = '" <<m_stReq.bm_id
              <<"' AND bill_date >= '" <<m_stReq.beg_date
              <<"' AND bill_date <= '" <<m_stReq.end_date <<"'";
        if (m_stReq.pay_channel != "")
        {
            count_sql <<" AND pay_channel = '" <<m_stReq.pay_channel <<"'";
            sqlss <<" AND pay_channel = '" <<m_stReq.pay_channel <<"'";
        }
        if (m_stReq.mch_id != "")
        {
            count_sql <<" AND mch_id = '" <<m_stReq.mch_id <<"'";
            sqlss <<" AND mch_id = '" <<m_stReq.mch_id <<"'";
        }
        if (m_stReq.mch_name != "")
        {
            count_sql <<" AND mch_name LIKE '%" <<m_stReq.mch_name <<"%' ";
            sqlss <<" AND mch_name LIKE '%" <<m_stReq.mch_name <<"%'";
        }
        if (m_stReq.shared_type == "")
        {
            count_sql <<" AND fund_type != 'mch' ";
            sqlss <<" AND fund_type != 'mch' ";
        }
        else
        {
            count_sql <<" AND fund_type = '" <<m_stReq.shared_type <<"'";
            sqlss <<" AND fund_type = '" <<m_stReq.shared_type <<"'";
        }
        sqlss << "ORDER BY bill_date LIMIT " <<(m_stReq.page-1)*m_stReq.page_count <<"," <<m_stReq.page_count;
    }

    SQL_TRY_BEG();
    ret = m_mysql.QryAndFetchResMVector(*m_pBillDb, sqlss.str().c_str(), outmap);
    SQL_TRY_END();
    JsonMap jlist;
    char index[8];
    if (ret == 1)
    {
        for (unsigned int i = 0; i < outmap.size(); ++i)
        {
            JsonMap jdata;
            sprintf(index, "%d", i);
            for (map<string, string>::iterator iter = outmap[i].begin(); iter != outmap[i].end(); ++iter)
            {
                jdata[JsonType(iter->first)] = JsonType(iter->second);
            }
            jlist[JsonType(index)] = JsonType(jdata);
        }
        SQL_TRY_BEG();
        ret = m_mysql.QryAndFetchResMap(*m_pBillDb, count_sql.str().c_str(), cm);
        SQL_TRY_END();
        m_jsonRsp[JsonType("count")] = JsonType(cm["ct"]);
    }
    else
    {
        CINFO_LOG("query but no data found");
    }
    

    m_jsonRsp[JsonType("content")] = JsonType(jlist);
}
