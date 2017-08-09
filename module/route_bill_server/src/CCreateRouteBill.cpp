/*
 * CAdminPermissionMng.cpp
 *
 *  Created on: 2017年7月11日
 *      Author: hawrkchen
 */
#include "CCreateRouteBill.h"
#include "json_util.h"

INT32 CCreateRouteBill::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
{
	CDEBUG_LOG("Begin ...");

    gettimeofday(&m_stStart, NULL);
    Reset();

    if ( !m_bInited )
    {
        snprintf( m_szErrMsg, sizeof(m_szErrMsg), "Not Inited" );
        m_iRetCode = -1;
        return m_iRetCode;
    }

	try
	{
        //获取请求
	    FillReq(mapInput);

	    //校验请求
	    CheckInput();

	    CalcEffectiveTimeBill();

	    GetLocalBillData();

		//设置返回参数---hawrk返回参数不明确，不建议用
		//SetRetParam();
	}
	catch(CTrsExp& e)
	{
		m_RetMap.clear();
		m_RetMap["err_code"] = e.retcode;
		m_RetMap["err_msg"]  = e.retmsg;
	}

    BuildResp( outbuf, outlen );

    CDEBUG_LOG("------------ process end----------");
    return atoi(m_RetMap["err_code"].c_str());
}

/*
 * 解析出请求结构
 * 成功返回0 失败返回-1
 */
void CCreateRouteBill::FillReq( NameValueMap& mapInput)
{
	NameValueMapIter iter;
	for(iter=mapInput.begin();iter!=mapInput.end();iter++)
	{
		string szName = iter->first;
		transform(szName.begin(), szName.end(), szName.begin(), ::tolower);
		//m_InParams[szName] = getSafeInput(iter->second);
		m_InParams[szName] = iter->second;
	}

	//CDEBUG_LOG("biz_content =[%s]",m_InParams["biz_content"].c_str());
//	cJSON* biz_content = cJSON_Parse(m_InParams["biz_content"].c_str());
//
//	JsonType jt = JsonUtil::stringToObject(m_InParams["biz_content"]);
//	JsonMap jm = jt.toMap();

	ParseJson2Map(m_InParams["biz_content"],m_InParams);


}

void CCreateRouteBill::CheckInput()
{

	if(m_InParams["ver"].empty() || m_InParams["ver"] != VER)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("bill_date", m_InParams["bill_date"], 1, 10, true);  //对账日期  20170710


	//Check::CheckStrParam("pay_channel", m_InParams["pay_channel"], 1, 10, true);  //银行通道


}

INT32 CCreateRouteBill::CalcEffectiveTimeBill()
{
	BEGIN_LOG(__func__);
//	if (m_InParams["bill_date"].empty())
//	{
//		m_InParams["bill_date"] = GetYesterDate();
//	}

	char szBuf[50] = { 0 };
	snprintf(szBuf, sizeof(szBuf), "%s %s", toDate(m_InParams["bill_date"]).c_str(), "00:00:00");
	m_start_time = szBuf;
	memset(szBuf,0x00,sizeof(szBuf));
	snprintf(szBuf, sizeof(szBuf), "%s %s", toDate(m_InParams["bill_date"]).c_str(), "23:59:59");
	m_end_time =szBuf;

	CDEBUG_LOG("CalcEffectiveTimeBill ! "
		"m_start_time:[%s] m_end_time:[%s].\n",
		m_start_time.c_str(),
		m_end_time.c_str());

	return 0;
}

INT32 CCreateRouteBill::GetLocalBillData()
{
	int iRet = 0;

	//先检验本地对账表是否已有今天的对账记录
	CheckBillRecord();

	//切换DB机器10张表  生成支付对账单文件
	CDBPool* pDbPool = Singleton<CSpeedPosConfig>::GetInstance()->GetDBPool();
	std::map<int, clib_mysql*> DbConMap = pDbPool->GetMasterDBPool();
	for (std::map<int, clib_mysql*>::iterator iter = DbConMap.begin(); iter != DbConMap.end(); ++iter)
	{
		bool bFirstTable = true;  //控制打印SQL
		clib_mysql* pDbCon = iter->second;
		for (int iDbIndex = 0; iDbIndex < 10; ++iDbIndex)
		{
			CDEBUG_LOG("pDbCon[%p] host[%s]! iDbIndex[%d]. \n", pDbCon, pDbCon->ms_host, iDbIndex);
			iRet = GetSysPaymentBill(pDbCon,iDbIndex,bFirstTable);
			if (iRet < 0)
			{
				CERROR_LOG("GetSysPaymentBill failed! Ret[%d] Err[%s].\n",iRet, "fail");
				throw(CTrsExp(iRet,"GetSysPaymentBill failed!"));

			}

			bFirstTable = false;
		}

		// 生成退款对账单文件
		GetSysRefundBill(pDbCon);

	}



	return 0;
}

INT32 CCreateRouteBill::GetSysPaymentBill(clib_mysql* order_db,const int& iDbIndex,bool bFirstTab)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	SqlResultMapVector resMVector;
	SqlResultMapVector resChanVector;
	char szOrderTableName[128] = { 0 };
	char szOrderChannelTableName[128] = { 0 };
	std::string strDataBaseSuffix = m_InParams["bill_date"].substr(0,6);
	snprintf(szOrderTableName, sizeof(szOrderTableName), "%s_%s.%s_%d", ROUTE_ORDER_DB,strDataBaseSuffix.c_str(),ORDER_TABLE,iDbIndex);
	snprintf(szOrderChannelTableName, sizeof(szOrderChannelTableName), "%s_%s.%s_%d", ROUTE_ORDER_DB,strDataBaseSuffix.c_str(),ORDER_CHANNEL,iDbIndex);

	CDEBUG_LOG("ordertable_name[%s],channel table name =[%s]!", szOrderTableName,szOrderChannelTableName);


	CDEBUG_LOG(" host[%s]! iDbIndex[%d]. \n", order_db->ms_host, iDbIndex);

	sqlss.str("");
	sqlss	<<" SELECT Forder_no,Fout_order_no,Fmch_id,Fmch_name, Fpay_platform,Fpay_type, Fpay_channel_id,Fpay_channel, Fchannel_mch_id,"
				<<" Ftransaction_id,Fpayment_fee,Frefund_fee,Ffactor_rate_val,Fmch_rate_val,Fproduct_rate_val,Fplatform_rate_val, "
				<<"Fcur_type,Forder_status,Fpay_time FROM "<< szOrderTableName
				<<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time
				<<"' AND Forder_status = 2";

	iRet = m_mysql.QryAndFetchResMVector(*order_db,sqlss.str().c_str(),resMVector);
	if(iRet == 1)  //有记录
	{
		ostringstream sqlss;
		for(size_t i = 0; i < resMVector.size(); i++)
		{
			sqlss.str("");
			sqlss <<"insert into "
				  <<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
				  <<" (Forder_no,Fout_order_no,Fagent_id,Fmch_id,Fmch_name,Fpay_platform,Fpay_type,Fpay_channel_id,Fpay_channel,Fchannel_mch_id,"
				  <<"Ftransaction_id,Ftotal_amount,Frefund_amount,Ffactor_rate_val,Fmch_rate_val,Fproduct_rate_val,Fplatform_rate_val,Fcur_type,"
				  <<"Forder_status,Fcheck_status,Fpay_time)"
				  <<" values('"<<resMVector[i]["Forder_no"]<<"','"<<resMVector[i]["Fout_order_no"]<<"','','"<<resMVector[i]["Fmch_id"]
				  <<"','"<<resMVector[i]["Fmch_name"]
				  <<"','"<<resMVector[i]["Fpay_platform"]<<"','"<<resMVector[i]["Fpay_type"]<<"','"<<resMVector[i]["Fpay_channel_id"]
				  <<"','"<<resMVector[i]["Fpay_channel"]<<"','"<<resMVector[i]["Fchannel_mch_id"]<<"','"<<resMVector[i]["Ftransaction_id"]
				  <<"','"<<resMVector[i]["Fpayment_fee"]<<"','"<<resMVector[i]["Frefund_fee"]<<"','"<<resMVector[i]["Ffactor_rate_val"]
				  <<"','"<<resMVector[i]["Fmch_rate_val"]<<"','"<<resMVector[i]["Fproduct_rate_val"]<<"','"<<resMVector[i]["Fplatform_rate_val"]
				  <<"','"<<resMVector[i]["Fcur_type"]<<"','SUCCESS',0,'"<<resMVector[i]["Fpay_time"]<<"');";

		    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
		    if(iRet != 1)
		    {
		    	CERROR_LOG("insert t_route_bill fail!!!");
		    	throw(CTrsExp(INSERT_DB_ERR,"insert t_route_bill fail!!!"));
		    }

		    //加渠道信息
		    sqlss.str("");
		    sqlss <<"select Forder_no,Fpay_channel_id,Fmch_id,Fpay_platform,Fpay_type,Ffactor_id, Ftotal_amount,Ffactor_rate_val from "
		    	  <<szOrderChannelTableName<<" WHERE Forder_no ='"<<resMVector[i]["Forder_no"]<<"'";

		    iRet = m_mysql.QryAndFetchResMVector(*order_db,sqlss.str().c_str(),resChanVector);
			for(size_t i = 0; i < resChanVector.size(); i++)
			{
				sqlss.str("");
				sqlss  <<"insert into "<<ROUTE_BILL_DB<<"."<<T_ROUTE_CHANNEL<<" (Forder_no,Fpay_channel_id,Fmch_id,Fpay_platform, Fpay_type,Fagent_id,"
					   <<"Ftotal_amount,Fchannel_profit, Forder_status, Fpay_time) values('"<<resChanVector[i]["Forder_no"]<<"','"<<resChanVector[i]["Fpay_channel_id"]
					   <<"','"<<resChanVector[i]["Fmch_id"]<<"','"<<resChanVector[i]["Fpay_platform"]<<"','"<<resChanVector[i]["Fpay_type"]
			           <<"','"<<resChanVector[i]["Ffactor_id"]<<"','"<<resChanVector[i]["Ftotal_amount"]<<"','"<<resChanVector[i]["Ffactor_rate_val"]
					   <<"','SUCCESS','"<<resMVector[i]["Fpay_time"]<<"');";
			    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
			    if(iRet != 1)
			    {
			    	CERROR_LOG("insert t_route_bill_channel fail!!!");
			    	throw(CTrsExp(INSERT_DB_ERR,"insert t_route_bill_channel fail!!!"));
			    }
			}

		}
	}
	return 0;

}


void CCreateRouteBill::GetSysRefundBill(clib_mysql* refund_db)
{
	INT32 iRet = 0;
	SqlResultMapVector resMVector;

	sqlss.str("");
	sqlss	<<" SELECT Frefund_no,Fout_refund_no,Fmch_id,Fmch_name, Fpay_platform,Fpay_type, Fpay_channel_id,Fpay_channel,"
				<<" Frefund_id,Frefund_fee,Frefund_status,Frefund_time FROM "
				<< ROUTE_REFUND_DB<<"."<<REFUND_TABLE
				<<" WHERE Frefund_time >='"<<m_start_time<<"' AND Frefund_time <='"<<m_end_time
				<<"' AND Frefund_status = 2";

	iRet = m_mysql.QryAndFetchResMVector(*refund_db,sqlss.str().c_str(),resMVector);
	if(iRet == 1)  //有记录
	{
		for(size_t i = 0; i < resMVector.size(); i++)
		{
			sqlss.str("");
			sqlss <<"insert into "
				  <<ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
				  <<" (Forder_no,Fout_order_no,Fagent_id,Fmch_id,Fmch_name,Fpay_platform,Fpay_type,Fpay_channel_id,Fpay_channel,"
				  <<"Ftransaction_id,Frefund_amount,Forder_status,Fcheck_status,Fpay_time)"
				  <<" values('"<<resMVector[i]["Frefund_no"]<<"','"<<resMVector[i]["Fout_refund_no"]<<"','','"<<resMVector[i]["Fmch_id"]
				  <<"','"<<resMVector[i]["Fmch_name"]
				  <<"','"<<resMVector[i]["Fpay_platform"]<<"','"<<resMVector[i]["Fpay_type"]<<"','"<<resMVector[i]["Fpay_channel_id"]
				  <<"','"<<resMVector[i]["Fpay_channel"]<<"','"<<resMVector[i]["Frefund_id"]
				  <<"','"<<resMVector[i]["Frefund_fee"]<<"','REFUND',0,'"<<resMVector[i]["Frefund_time"]<<"');";

		    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
		    if(iRet != 1)
		    {
		    	CERROR_LOG("insert t_route_check_bill fail!!!");
		    	throw(CTrsExp(INSERT_DB_ERR,"insert t_route_check_bill fail!!!"));
		    }

		    //加渠道信息   -----hawrk 先不考虑退款退手续费
//			sqlss.str("");
//			sqlss	<<" insert into "<<ROUTE_BILL_DB<<"."<<ORDER_CHANNEL<<" (Forder_no,Fpay_channel_id,Fmch_id,Fpay_platform, Fpay_type,Fagent_id, "
//						<<"Ftotal_amount,Fchannel_profit, Forder_status, Fpay_time) select Frefund_no,Fpay_channel_id,Fmch_id,Fpay_platform, "
//						<<"Fpay_type,Fagent_id, Frefund_fee,Fchannel_profit, 'SUCCESS',Fpay_time from "<< ROUTE_REFUND_DB<<"."<<REFUND_TABLE
//						<<" WHERE Forder_no ='"<<resMVector[i]["Forder_no"]<<"'";
//
//		    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
//		    if(iRet != 1)
//		    {
//		    	CERROR_LOG("insert t_route_bill_channel fail!!!");
//		    	throw(CTrsExp(INSERT_DB_ERR,"insert t_route_bill_channel fail!!!"));
//		    }
		}
	}
}

void CCreateRouteBill::CheckBillRecord()
{
	INT32 iRet = 0;
	SqlResultSet outMap;

	sqlss.str("");
	sqlss	<<" SELECT count(*) as count FROM "
				<< ROUTE_BILL_DB<<"."<<T_ROUTE_BILL
				<<" WHERE Fpay_time >='"<<m_start_time<<"' AND Fpay_time <='"<<m_end_time<<"';";

	iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),outMap);
	if(iRet < 0)
	{
		CERROR_LOG("get check bill fail!!");
		throw(CTrsExp(QRY_DB_ERR,"get check bill fail!!"));
	}
	if(atoi(outMap["count"].c_str()) > 0)
	{
		CERROR_LOG("check bill record exist!!");
		throw(CTrsExp(ERR_CHECK_BILL_EXIST,"本地对账记录已生成!!"));
	}
}

void CCreateRouteBill::SetRetParam()
{
//	CDEBUG_LOG("Begin ...");
//
//	for(NameValueMapIter iter = m_RetMap.begin(); iter != m_RetMap.end(); iter++)
//	{
//		m_ContentJsonMap.insert(JsonMap::value_type(JsonType(iter->first), JsonType(iter->second)));
//	}
//
//	CDEBUG_LOG("End.");
}

void CCreateRouteBill::BuildResp( CHAR** outbuf, INT32& outlen )
{
	CDEBUG_LOG("Begin ...");

    CHAR szResp[ MAX_RESP_LEN ];
	JsonMap jsonRsp;

	if(m_RetMap["err_code"].empty())
	{
		m_RetMap["err_code"] = "0";
		m_RetMap["err_msg"]  = "success";
	}
//	else
//    {
//        Singleton<CSpeedPosConfig>::GetInstance()->GetBusiConf()->ReadErrCodeFromFile(m_RetMap);
//    }

	jsonRsp.insert(JsonMap::value_type(JsonType("ret_code"), JsonType(m_RetMap["err_code"])));
	jsonRsp.insert(JsonMap::value_type(JsonType("ret_msg"), JsonType(m_RetMap["err_msg"])));
	jsonRsp.insert(JsonMap::value_type(JsonType("biz_content"), JsonType(m_ContentJsonMap)));

	std::string resContent = JsonUtil::objectToString(jsonRsp);

	snprintf(szResp, sizeof(szResp), //remaincount=1
		"%s\r\n",
		resContent.c_str());

	outlen = strlen(szResp);
	*outbuf = (char*)malloc(outlen);
	memcpy(*outbuf, szResp, outlen);

	CDEBUG_LOG("Rsp :[%s]",szResp);
	CDEBUG_LOG("-----------time userd:[%d ms]---------",SpeedTime());
}

void CCreateRouteBill::LogProcess()
{
}


