/*
 * CBillConstrastSPDB.cpp
 *
 *  Created on: 2017年6月8日
 *      Author: hawrkchen
 */
#include "CBillConstrastSPDB.h"
#include "CCommFunc.h"

extern CSpeedPosServer g_cOrderServer;

CBillContrastSPDB::CBillContrastSPDB()
{
	CDEBUG_LOG("CBillContrastSPDB begin");
	pBillDb = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
}

CBillContrastSPDB::~CBillContrastSPDB()
{
	CDEBUG_LOG("~CBillContrastSPDB begin");
}

void CBillContrastSPDB::InitBillFileDownLoad(ProPullBillReq& m_stReq,int starttime)
{
	int iRet = -1;
	clib_mysql* pBillDb = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
	CMySQL m_mysql;
	ostringstream sqlss;
	SqlResultSet outmap;

    sqlss.str("");
    sqlss <<"select bill_date,pay_channel,bill_status"
    	  <<" from "<<BILL_DB<<"."<<BILL_DOWNLOAD
		  <<" where bm_id ='"<<m_stReq.sBmId<<"' and bill_date='"<<nowdate(m_stReq.sInputTime)
		  <<"' and pay_channel ='"<<m_stReq.sPayChannel<<"';";

    iRet = m_mysql.QryAndFetchResMap(*pBillDb,sqlss.str().c_str(),outmap);
    if(iRet == 1)
    {
    	//有数据
    	if(outmap["bill_status"] == "2")
    	{
    		CERROR_LOG("bill file has been download success!!!");
    		throw(CTrsExp(ERR_DOWNLOAD_FILE_EXIST,errMap[ERR_DOWNLOAD_FILE_EXIST]));
    	}
    	else
    	{
    		CDEBUG_LOG("bill_download status :fail!!");
    		return;
    	}
    }
    //重新下载请求
    sqlss.str("");
    sqlss <<"insert into "
    	  <<BILL_DB<<"."<<BILL_DOWNLOAD
		  <<" (bm_id,bill_date,pay_channel,bank_inscode,bill_status,operator_id)"
		  <<" values('"<<m_stReq.sBmId<<"','"<<nowdate(m_stReq.sInputTime)<<"','"<<m_stReq.sPayChannel
		  <<"','"<<GetPayChannelCode(m_stReq)<<"','1','"<<m_stReq.sOperator<<"');";

    iRet = m_mysql.Execute(*pBillDb,sqlss.str().c_str());
    if(iRet != 1)
    {
    	CERROR_LOG("insert bill_db.bill_download fail!!!");
    	throw(CTrsExp(ERR_DB_INSERT,"insert bill_db.bill_download fail!!!"));
    }

}

void CBillContrastSPDB::BillFileDownLoad(ProPullBillReq& m_stReq,int starttime)
{
	const CBillBusiConfig::BankAttr* p_bank_attr = pBillBusConfig->GetBankAttrCfg(m_stReq.sBmId);
	try
	{
		if(p_bank_attr->strUseSftp == "0")
		{
			Copy2GetFile(m_stReq,starttime);
		}
		else
		{
			SFTPDownLoad(m_stReq,starttime);
		}
	}
	catch(tars::TC_File_Exception& e)
	{
		CERROR_LOG("use sftp/copy to get bill file  fail!!!");
		throw(CTrsExp(ERR_DOWNLOAD_FILE,errMap[ERR_DOWNLOAD_FILE]));
	}

   //wx
	if(m_stReq.sPayChannel == WX_API_PAY_CHANNEL)
	{
		if(access(sWxBillSrcFileName.c_str(),F_OK) == 0) //文件存在
		{
			UpdateDownLoadDB(m_stReq,2,sWxBillSrcFileName.c_str());
		}
		else
		{
			UpdateDownLoadDB(m_stReq,3,"");
		}
	}
    //ali
	if(m_stReq.sPayChannel == ALI_API_PAY_CHANNEL)
	{
		if(access(sAliBillSrcFileName.c_str(),F_OK) == 0)
		{
			UpdateDownLoadDB(m_stReq,2,sAliBillSrcFileName.c_str());
		}
		else
		{
			UpdateDownLoadDB(m_stReq,3,"");
		}
	}

}

void CBillContrastSPDB::LoadBillFlowToDB(ProPullBillReq& m_stReq,int starttime)
{
	BEGIN_LOG(__func__);
	CDEBUG_LOG("LoadBillFlowToDB begin");
	INT32 iRet = 0;

	if(access(sShopBillSrcFileName.c_str(),F_OK) != 0
			||access(sChannelBillSrcFileName.c_str(),F_OK) != 0)
	{
		CERROR_LOG("download file not exist!!");
		throw(CTrsExp(ERR_DETAIL_FILE_NOT_FOUND,errMap[ERR_DETAIL_FILE_NOT_FOUND]));
	}
	if(m_stReq.sPayChannel == WX_API_PAY_CHANNEL)
	{
		if(access(sWxBillSrcFileName.c_str(),F_OK) != 0)
		{
			CERROR_LOG("wx bill file not exist!!");
			throw(CTrsExp(ERR_DETAIL_FILE_NOT_FOUND,"wx bill file not exist!!"));
		}
	}
	if(m_stReq.sPayChannel == ALI_API_PAY_CHANNEL)
	{
		if(access(sAliBillSrcFileName.c_str(),F_OK) != 0)
		{
			CERROR_LOG("ali bill file not exist!!");
			throw(CTrsExp(ERR_DETAIL_FILE_NOT_FOUND,"ali bill file not exist!!"));
		}
	}

	//写对账批次管理表
	InitBillBatchDB(m_stReq);

	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;
	std::string strDbName = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDbName();


	//清表
	iRet = m_stTransFlowDao.TruncateEveryPaymentTypeSysFlowData(*pBillDb,m_stReq.sBmId,m_stReq.sPayChannel);
	if(iRet < 0)
	{
		CERROR_LOG("Truncate Table Fail!!!");
		throw(CTrsExp(SYSTEM_ERR,"Truncate Table Fail!!!"));
	}

	std::string sPath = mainConfig.sSftpShellPath;

	char szLoadOrderFlowCmd[512] = { 0 };
	snprintf(szLoadOrderFlowCmd, sizeof(szLoadOrderFlowCmd), "sh %s%s %s %d %s %s %s %s %s",
		sPath.c_str(), mainConfig.sBillTruncateDBShellPath.c_str(), pBillDb->ms_host, pBillDb->mi_port,
		pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(),sShopBillSrcFileName.c_str(),ORDER_ALL_FLOW);
	CDEBUG_LOG("szLoadOrderFlowCmd = [%s]",szLoadOrderFlowCmd);
	system(szLoadOrderFlowCmd);

	memset(szLoadOrderFlowCmd,0x00,sizeof(szLoadOrderFlowCmd));
	snprintf(szLoadOrderFlowCmd, sizeof(szLoadOrderFlowCmd), "sh %s%s %s %d %s %s %s %s %s",
		sPath.c_str(), mainConfig.sBillTruncateDBShellPath.c_str(), pBillDb->ms_host, pBillDb->mi_port,
		pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(),sChannelBillSrcFileName.c_str(),ORDER_CHANNEL_FLOW);
	CDEBUG_LOG("szLoadOrderFlowCmd = [%s]",szLoadOrderFlowCmd);
	system(szLoadOrderFlowCmd);

	if (0 == strcmp(m_stReq.sPayChannel.c_str(), WX_API_PAY_CHANNEL))
	{
		memset(szLoadOrderFlowCmd,0x00,sizeof(szLoadOrderFlowCmd));
		snprintf(szLoadOrderFlowCmd, sizeof(szLoadOrderFlowCmd), "sh %s%s %s %d %s %s %s %s %s",
			sPath.c_str(), mainConfig.sBillTruncateDBShellPath.c_str(), pBillDb->ms_host, pBillDb->mi_port,
			pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(),sWxBillSrcFileName.c_str(),BILL_WXPAY_FLOW);
		CDEBUG_LOG("szLoadOrderFlowCmd = [%s]",szLoadOrderFlowCmd);
		system(szLoadOrderFlowCmd);
	}

	if (0 == strcmp(m_stReq.sPayChannel.c_str(), ALI_API_PAY_CHANNEL))
	{
		memset(szLoadOrderFlowCmd,0x00,sizeof(szLoadOrderFlowCmd));
		snprintf(szLoadOrderFlowCmd, sizeof(szLoadOrderFlowCmd), "sh %s%s %s %d %s %s %s %s %s",
			sPath.c_str(), mainConfig.sBillTruncateDBShellPath.c_str(), pBillDb->ms_host, pBillDb->mi_port,
			pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(),sAliBillSrcFileName.c_str(),BILL_ALIPAY_FLOW);
		CDEBUG_LOG("szLoadOrderFlowCmd = [%s]",szLoadOrderFlowCmd);
		system(szLoadOrderFlowCmd);
	}



	//文件装载到DB
//	iRet = m_stTransFlowDao.LoadFiletoDB(*pBillDb,sShopBillSrcFileName,ORDER_ALL_FLOW);
//	if(iRet < 0)
//	{
//		CERROR_LOG("load file to shop DB fail!!!");
//		throw(CTrsExp(SYSTEM_ERR,"load file to shop DB fail!!!"));
//	}
//
//	iRet = m_stTransFlowDao.LoadFiletoDB(*pBillDb,sChannelBillSrcFileName,ORDER_CHANNEL_FLOW);
//	if(iRet < 0)
//	{
//		CERROR_LOG("load file to channel DB fail!!!");
//		throw(CTrsExp(SYSTEM_ERR,"load file to channel DB fail!!!"));
//	}
//
//	if (0 == strcmp(m_stReq.sPayChannel.c_str(), WX_API_PAY_CHANNEL))
//	{
//		iRet = m_stTransFlowDao.LoadFiletoDB(*pBillDb,sWxBillSrcFileName,BILL_WXPAY_FLOW);
//		if(iRet < 0)
//		{
//			CERROR_LOG("load file to wx DB fail!!!");
//			throw(CTrsExp(SYSTEM_ERR,"load file to wx DB fail!!!"));
//		}
//	}
//	if (0 == strcmp(m_stReq.sPayChannel.c_str(), ALI_API_PAY_CHANNEL))
//	{
//		iRet = m_stTransFlowDao.LoadFiletoDB(*pBillDb,sAliBillSrcFileName,BILL_ALIPAY_FLOW);
//		if(iRet < 0)
//		{
//			CERROR_LOG("load file to ali DB fail!!!");
//			throw(CTrsExp(SYSTEM_ERR,"load file to ali DB fail!!!"));
//		}
//	}
}

void CBillContrastSPDB::ProcBillComparison(ProPullBillReq& m_stReq,int starttime,int endtime)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	CDEBUG_LOG("ProcBillComparison: Begin\n");

	/** 计算开始时间  结束时间**/
	std::string sBeginTime = getSysTime(starttime);
	std::string sEndTime = getSysTime(endtime);
	//CDEBUG_LOG("ProcBillComparison: BeginTime[%s] EndTime[%s]\n", sBeginTime.c_str(), sEndTime.c_str());
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;


	/**-----------分离微信pay和支付宝支付订单流水------------*/
	if (0 == strcmp(m_stReq.sPayChannel.c_str(), WX_API_PAY_CHANNEL))
	{

		iRet = m_stTransFlowDao.InsertTradeTypeOrderToDB(*pBillDb,m_stReq.sBmId,WX_API_PAY_CHANNEL,
														ORDER_WXPAY_FLOW);
		if (iRet < 0)
		{
			CERROR_LOG("Wxpay InsertTradeTypeOrderToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());

		}

		iRet = m_stTransFlowDao.InsertTradeTypeOrderChannelToDB(*pBillDb,m_stReq.sBmId,WX_API_PAY_CHANNEL,
															ORDER_WX_CHAN_FLOW);
		if (iRet < 0)
		{
			CERROR_LOG("Wxpay InsertTradeTypeOrderChannelToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}

	}

	if (0 == strcmp(m_stReq.sPayChannel.c_str(), ALI_API_PAY_CHANNEL))
	{
		//去除支付宝差异状态
		if(m_stTransFlowDao.RemoveAlipayDifferenceSuccState(*pBillDb,m_stReq.sBmId) < 0
			|| m_stTransFlowDao.RemoveAlipayDifferenceRefundState(*pBillDb,m_stReq.sBmId) < 0)
		{
			CERROR_LOG("Wxpay RemoveAlipayDifferenceSuccState failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}

		iRet = m_stTransFlowDao.InsertTradeTypeOrderToDB(*pBillDb,m_stReq.sBmId,ALI_API_PAY_CHANNEL,
														ORDER_ALIPAY_FLOW);
		if (iRet < 0)
		{
			CERROR_LOG("ALI InsertTradeTypeOrderToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}


		iRet = m_stTransFlowDao.InsertTradeTypeOrderChannelToDB(*pBillDb,m_stReq.sBmId,ALI_API_PAY_CHANNEL,
														ORDER_ALI_CHAN_FLOW);
		if (iRet < 0)
		{
			CERROR_LOG("ALI InsertTradeTypeOrderChannelToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
	}
	/****----------------------------------------------------------------***/

	/*--------------empty 当天数据 防止重复当天对账产生垃圾数据 -----------------*/
	if(m_stTransFlowDao.EmptyTableData(*pBillDb,BILL_SUCC_FLOW,m_stReq.sBmId,sBeginTime,sEndTime) < 0
		|| m_stTransFlowDao.EmptyTableData(*pBillDb,BILL_WX_OVERFLOW,m_stReq.sBmId,sBeginTime,sEndTime) < 0
		|| m_stTransFlowDao.EmptyTableData(*pBillDb,BILL_ALI_OVERFLOW,m_stReq.sBmId,sBeginTime,sEndTime) < 0)
	{
		CERROR_LOG("EmptyTableData failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
		throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
	}
	/****-------------------------------------------------------------------***/

	/*** ---------------找出微信支付 相同的数据和不同数据---------------------------**/
	if (0 == strcmp(m_stReq.sPayChannel.c_str(), WX_API_PAY_CHANNEL))
	{
		//本地和微信都是成功
		iRet = m_stTransFlowDao.InsertPayIdenticalWxToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime);
		if (iRet < 0)
		{
			CERROR_LOG("InsertPayIdenticalWxToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
		//本地和微信都是退款
		iRet = m_stTransFlowDao.InsertRefundIdenticalWxToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime);
		if (iRet < 0)
		{
			CERROR_LOG("InsertRefundIdenticalWxToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}

		//本地和微信都有，但金额不符
		iRet = m_stTransFlowDao.InsertAmountNotMatchToDB(*pBillDb,m_stReq.sBmId,m_stReq.sPayChannel,
							nowdate(m_stReq.sInputTime),m_batch_no,sBeginTime,sEndTime);
		if(iRet < 0)
		{
			CERROR_LOG("InsertAmountNotMatchToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}

		//微信方 成功的多
		iRet = m_stTransFlowDao.InsertPayDistinctWxToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime);
		if (iRet < 0)
		{
			CERROR_LOG("InsertPayDistinctWxToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
		//微信方退款的多
		iRet = m_stTransFlowDao.InsertRefundDistinctWxToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime);
		if (iRet < 0)
		{
			CERROR_LOG("InsertRefundDistinctWxToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}

		//本地成功的多
		iRet = m_stTransFlowDao.InsertPaySuccessToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime,m_stReq.sPayChannel);
		if (iRet < 0)
		{
			CERROR_LOG("InsertPayDistinctWxToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
		//本地退款的多
		iRet = m_stTransFlowDao.InsertPayRefundToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime,m_stReq.sPayChannel);
		if (iRet < 0)
		{
			CERROR_LOG("InsertPayDistinctWxToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}

		//校验微信溢出
		std::vector<WxFlowSummary> wxOverFlowVec;
		iRet = m_stTransFlowDao.GetWxOverFlowData(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime,wxOverFlowVec);
		if (iRet < 0)
		{
			CERROR_LOG("GetWxOverFlowData failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
		CDEBUG_LOG("wxOverFlowVec [%d]\n", wxOverFlowVec.size());
		if (wxOverFlowVec.size())//检测有溢出的数据
		{
			for (size_t iIndex = 0; iIndex < wxOverFlowVec.size(); ++iIndex)
			{
				//写异常表
				iRet = m_stTransFlowDao.InsertwxAbnormalDB(*pBillDb,m_stReq.sBmId,nowdate(m_stReq.sInputTime),m_batch_no,m_stReq.sPayChannel,wxOverFlowVec,iIndex);
				if(iRet < 0)
				{
					CERROR_LOG("InsertAbnormalDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
					throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
				}
				if(wxOverFlowVec[iIndex].overflow_type == "2")//渠道多
				{

					StringMap paramMap;
					std::string strBody;
					CDEBUG_LOG("Wxpay OverFlow order_no:[%s]\n", wxOverFlowVec[iIndex].order_no.c_str());

					//paramMap.insert(StringMap::value_type("ver", "1"));
					paramMap.insert(StringMap::value_type("sync_flag", "1"));   //同步更新
					paramMap.insert(StringMap::value_type("order_no", wxOverFlowVec[iIndex].order_no));
					paramMap.insert(StringMap::value_type("order_status", wxOverFlowVec[iIndex].order_status));
					if(wxOverFlowVec[iIndex].order_status == "REFUND")
					{
						paramMap.insert(StringMap::value_type("refund_status", wxOverFlowVec[iIndex].refund_status));
						paramMap.insert(StringMap::value_type("refund_no", wxOverFlowVec[iIndex].refund_no));
					}

					iRet = g_cOrderServer.SendRequestTradeServapi(mainConfig.sSignKey,mainConfig.sTradeServQryOrderUrl,paramMap,strBody);

					if(strBody.empty())
					{
						CERROR_LOG("Query TradeService no response!");
						throw CTrsExp(ERR_PASE_RETRUN_DATA,"Query TradeService no response!");
					}

					ProcOverFlowData(strBody,ORDER_WX_CHAN_FLOW,m_stReq);

				}

			}
			//throw(CTrsExp(ERR_WXPAY_NOT_RECONCLIED,errMap[ERR_WXPAY_NOT_RECONCLIED]));
		}
	}
	/****-------------------------------------------------------------------***/

	/*** ---------------找出支付宝支付 相同的数据和不同数据---------------------------**/
	if (0 == strcmp(m_stReq.sPayChannel.c_str(), ALI_API_PAY_CHANNEL))
	{
		//本地和支付宝都是成功
		iRet = m_stTransFlowDao.InsertAliPayIdenticalToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime,"SUCCESS");
		if (iRet < 0)
		{
			CERROR_LOG("InsertAliPayIdenticalToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
		//本地和支付宝都是退款
		iRet = m_stTransFlowDao.InsertAliPayIdenticalRefundToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime);
		if (iRet < 0)
		{
			CERROR_LOG("InsertAliPayIdenticalRefundToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
		//本地和支付宝都有，但金额不符
		iRet = m_stTransFlowDao.InsertAmountNotMatchToDB(*pBillDb,m_stReq.sBmId,m_stReq.sPayChannel,
							nowdate(m_stReq.sInputTime),m_batch_no,sBeginTime,sEndTime);
		if(iRet < 0)
		{
			CERROR_LOG("InsertAmountNotMatchToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
		//支付宝成功的多
		iRet = m_stTransFlowDao.InsertAliPayDistinctToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime,"SUCCESS");
		if (iRet < 0)
		{
			CERROR_LOG("InsertAliPayDistinctToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
		//支付宝退款的多
		iRet = m_stTransFlowDao.InsertAliPayDistinctRefundToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime);
		if (iRet < 0)
		{
			CERROR_LOG("InsertAliPayDistinctRefundToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}

		//本地成功的多
		iRet = m_stTransFlowDao.InsertPaySuccessToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime,m_stReq.sPayChannel);
		if (iRet < 0)
		{
			CERROR_LOG("InsertPayDistinctWxToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
		//本地退款的多
		iRet = m_stTransFlowDao.InsertPayRefundToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime,m_stReq.sPayChannel);
		if (iRet < 0)
		{
			CERROR_LOG("InsertPayDistinctWxToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}

		std::vector<AliFlowSummary> aliOverFlowVec;
		iRet = m_stTransFlowDao.GetAliOverFlowData(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime,aliOverFlowVec);
		if (iRet < 0)
		{
			CERROR_LOG("GetAliOverFlowData failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
		CDEBUG_LOG("aliOverFlowVec [%d]\n", aliOverFlowVec.size());
		if (aliOverFlowVec.size())//检测有溢出的数据
		{

			for (size_t iIndex = 0; iIndex < aliOverFlowVec.size(); ++iIndex)
			{
				//写异常表
				iRet = m_stTransFlowDao.InsertaliAbnormalDB(*pBillDb,m_stReq.sBmId,nowdate(m_stReq.sInputTime),m_batch_no,m_stReq.sPayChannel,aliOverFlowVec,iIndex);
				if(iRet < 0)
				{
					CERROR_LOG("InsertAbnormalDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
					throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
				}
				if(aliOverFlowVec[iIndex].overflow_type == "2")//渠道多
				{

					StringMap paramMap;
					std::string strBody;
					CDEBUG_LOG("Wxpay OverFlow order_no:[%s]\n", aliOverFlowVec[iIndex].order_no.c_str());
					paramMap.insert(StringMap::value_type("sync_flag", "1"));   //同步更新
					paramMap.insert(StringMap::value_type("order_no", aliOverFlowVec[iIndex].order_no));
					paramMap.insert(StringMap::value_type("order_status", aliOverFlowVec[iIndex].order_status));
					paramMap.insert(StringMap::value_type("refund_no", aliOverFlowVec[iIndex].refund_no));

					iRet = g_cOrderServer.SendRequestTradeServapi(mainConfig.sSignKey,mainConfig.sTradeServQryOrderUrl,paramMap,strBody);
					if(strBody.empty())
					{
						CERROR_LOG("Query TradeService no response!");
						throw CTrsExp(ERR_PASE_RETRUN_DATA,"Query TradeService no response!");
					}

					ProcOverFlowData(strBody,ORDER_ALI_CHAN_FLOW,m_stReq);
				}

			}

		}
	}

	//统计本地order 表和wx/ali 表的记录，统计到对账汇总表中
	iRet = m_stTransFlowDao.InsertSummaryDB(*pBillDb,m_stReq.sBmId,nowdate(m_stReq.sInputTime),m_batch_no,sBeginTime,sEndTime,m_stReq.sPayChannel.c_str());
	if (iRet < 0)
	{
		CERROR_LOG("InsertSummaryDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
		throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
	}

}

void CBillContrastSPDB::GetRemitBillData(ProPullBillReq& m_stReq,int starttime,int endtime)
{
	CDEBUG_LOG("GetRemitBillData begin");
	INT32 iRet = 0;
	std::string sBeginTime = getSysTime(starttime);
	std::string sEndTime = getSysTime(endtime);

	//取成功汇总
	iRet = m_stTransFlowDao.GetPayBillData(*pBillDb,
			m_stReq.sBmId,m_stReq.sPayChannel,
		sBeginTime,
		sEndTime,
		orderPayBillMap);
	if (iRet < 0)
	{
		CERROR_LOG("GetPayBillData failed!iRet =[%d]", iRet);
		throw(CTrsExp(ERR_QUERY_RECORD_ERR,errMap[ERR_QUERY_RECORD_ERR]));
	}

	//取退款汇总
	iRet = m_stTransFlowDao.GetRefundBillData(*pBillDb,
			m_stReq.sBmId,m_stReq.sPayChannel,
		sBeginTime,
		sEndTime,
		orderRefundBillMap);
	if (iRet < 0)
	{
		CERROR_LOG("GetRefundBillData failed!iRet =[%d]", iRet);
		throw(CTrsExp(ERR_QUERY_RECORD_ERR,errMap[ERR_QUERY_RECORD_ERR]));
	}

	//计算渠道
	/**---wxpay and alipay channel-------------*/
	std::string strTableFix;

	if (m_stReq.sPayChannel == WX_API_PAY_CHANNEL)
	{
		strTableFix = "t_order_channel_wxpay_flow";
	}
	else if (m_stReq.sPayChannel == ALI_API_PAY_CHANNEL)
	{
		strTableFix = "t_order_channel_alipay_flow";
	}

	iRet = m_stTransFlowDao.GetChannelBillData(*pBillDb,
		strTableFix,
		m_stReq.sBmId,
		m_stReq.sPayChannel,
		sBeginTime,
		sEndTime,
		"SUCCESS",
		channelPayMap);
	if (iRet < 0)
	{
		CERROR_LOG("Get SUCCESS ChannelBillData failed!iRet =[%d]", iRet);
		throw(CTrsExp(ERR_QUERY_RECORD_ERR,errMap[ERR_QUERY_RECORD_ERR]));
	}

	iRet = m_stTransFlowDao.GetChannelBillData(*pBillDb,
		strTableFix,
		m_stReq.sBmId,
		m_stReq.sPayChannel,
		sBeginTime,
		sEndTime,
		"REFUND",
		channelRefundMap);
	if (iRet < 0)
	{
		CERROR_LOG("Get REFUND ChannelBillData failed!iRet =[%d]", iRet);
		throw(CTrsExp(ERR_QUERY_RECORD_ERR,errMap[ERR_QUERY_RECORD_ERR]));
	}
}
void CBillContrastSPDB::ProRemitBillProcess(ProPullBillReq& m_Req,int starttime)
{
	BEGIN_LOG(__func__);
	CDEBUG_LOG("ProRemitBillProcess begin");
	INT32 iRet = 0;
	//int iIndex = 1;
	int total_bm_profit = 0;
	int total_service_profit = 0;
	//int total_amount = 0;
	CMySQL m_mysql;
	/** 计算开始时间  结束时间**/
	std::string strPayTime = getSysDate(starttime);

	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;
	OrderStat orderStat;
	orderStat.Reset();
	string channel_code = GetPayChannelCode(m_Req);

	try
	{
		//开始事务
		m_mysql.Begin(*pBillDb);

		//清分商户
		CDEBUG_LOG("order trade bill num:[%d],refund bill num:[%d]",orderPayBillMap.size(),orderRefundBillMap.size());
		std::map<std::string, OrderRefundBillSumary>::iterator itRefund;
		for (std::map<std::string, OrderPayBillSumary>::iterator iter = orderPayBillMap.begin();
			iter != orderPayBillMap.end(); ++iter)
		//for(auto iter : orderPayBillMap)
		{
			orderStat.Reset();
			std::string mch_id = iter->first;
			orderStat.mch_id = mch_id;
			orderStat.trade_count = iter->second.trade_count;
			orderStat.trade_amount = iter->second.total_fee;
			orderStat.trade_net_amount = iter->second.total_fee;
			orderStat.total_net_commission = iter->second.total_commission;
			orderStat.shop_net_amount = iter->second.shop_amount;
			orderStat.payment_net_profit = iter->second.payment_profit;
			orderStat.channel_net_profit = iter->second.channel_profit;
			orderStat.service_net_profit = iter->second.service_profit;
			orderStat.bm_net_profit = iter->second.bm_profit;
			if ((itRefund = orderRefundBillMap.find(mch_id)) != orderRefundBillMap.end())
			{
				orderStat.refund_count = itRefund->second.refund_count;
				orderStat.refund_amount = itRefund->second.refund_fee;
				orderStat.trade_net_amount -= itRefund->second.refund_fee;
				orderStat.total_net_commission -= itRefund->second.total_commission;
				orderStat.shop_net_amount -= itRefund->second.shop_amount;
				orderStat.payment_net_profit -= itRefund->second.payment_profit;  //成本手续费
				orderStat.channel_net_profit -= itRefund->second.channel_profit;
				orderStat.service_net_profit -= itRefund->second.service_profit;
				orderStat.bm_net_profit -= itRefund->second.bm_profit;
			}
			//orderStat.trade_count  交易笔数
			//orderStat.trade_amount 交易金额
			//orderStat.trade_net_amount  交易净额
			//orderStat.shop_net_amount 待结算金额
			//orderStat.refund_count  退款笔数
			//orderStat.refund_amount 退款金额


			total_bm_profit += orderStat.bm_net_profit;
			total_service_profit += orderStat.service_net_profit;
			CDEBUG_LOG("OrderStat: mch_id[%s] trade_count[%d] trade_amount[%d] refund_count[%d] refund_amount[%d] trade_net_amount[%d]"
				" total_net_commission[%d] shop_net_amount[%d] payment_net_profit[%d] channel_net_profit[%d] service_net_profit[%d] bm_net_profit[%d] \n",
				orderStat.mch_id.c_str(), orderStat.trade_count, orderStat.trade_amount, orderStat.refund_count, orderStat.refund_amount,
				orderStat.trade_net_amount, orderStat.total_net_commission, orderStat.shop_net_amount, orderStat.payment_net_profit,
				orderStat.channel_net_profit, orderStat.service_net_profit, orderStat.bm_net_profit);

			TRemitBill remitBill;
			remitBill.Reset();
			remitBill.account_id = orderStat.mch_id;
			remitBill.sType = SHOP_TYPE_NAME;
			remitBill.remit_fee = orderStat.shop_net_amount;
			remitBill.sRemitfee = F2Y(toString(orderStat.shop_net_amount));
			remitBill.sPayTime = toDate(strPayTime);
			remitBill.sRemitTime = toDate(getSysDate());
			remitBill.sRemark = SHOP_REMARK_NAME;

			iRet = g_cOrderServer.CallGetBankNoApi(remitBill);
			if (iRet < 0)
			{
				CERROR_LOG("CallGetBankNoApi failed! iRet[%d].\n",iRet);
				throw(CTrsExp(ERR_NOTIFY_SETTLE_FAILED,"CallGetBankNoApi failed!"));
			}
			iRet = m_stTransFlowDao.InsertDistributionDB(*pBillDb,m_Req.sBmId,nowdate(m_Req.sInputTime),
									m_batch_no,m_Req.sPayChannel,channel_code,orderStat,remitBill,SHOP_TYPE_NAME);
			if (iRet < 0)
			{
				CERROR_LOG("InsertDistributionDB mch failed!iRet =[%d]", iRet);
				throw(CTrsExp(ERR_DB_UPDATE,"InsertDistributionDB  mch failed!"));
			}

			if (0 < orderStat.shop_net_amount)
			{

				//写结算表
				iRet = m_stTransFlowDao.InsertSettleDB(*pBillDb,m_Req.sBmId,nowdate(m_Req.sInputTime),
											m_batch_no,m_Req.sPayChannel,remitBill);
				if (iRet < 0)
				{
					CERROR_LOG("InsertSettleDB failed! iRet[%d].\n",iRet);
					throw(CTrsExp(ERR_DB_UPDATE,"InsertSettleDB failed!"));
				}

				//AddSettleLog(m_Req,remitBill,tremitMap,iIndex,total_amount);
			}
		}

		//清分渠道
		CDEBUG_LOG("bill channel trade num:[%d],refund num:[%d]",channelPayMap.size(),channelRefundMap.size());
		std::map<std::string, OrderChannelFlowData>::iterator it_refund;
		for (std::map<std::string, OrderChannelFlowData>::iterator iterCl = channelPayMap.begin(); iterCl != channelPayMap.end(); ++iterCl)
		{
			orderStat.Reset();
			std::string channel_id = iterCl->first;
			int channel_profit = iterCl->second.channel_profit;
			CDEBUG_LOG("pay channel_id:[%s] channel_profit[%d]\n", channel_id.c_str(), channel_profit);
			if ((it_refund = channelRefundMap.find(channel_id)) != channelRefundMap.end())
			{
				channel_profit -= it_refund->second.channel_profit;
				orderStat.refund_count = it_refund->second.total_count;
				orderStat.refund_amount = it_refund->second.refund_fee;
				CDEBUG_LOG("refund :channel_id=[%s], channel_profit=[%d],refund_count=[%d],refund_amount=[%d]\n",
						channel_id.c_str(), it_refund->second.channel_profit,orderStat.refund_count,orderStat.refund_amount);
			}


//			CDEBUG_LOG("out :refund_count=[%d],refund_amount=[%d]\n",
//					orderStat.refund_count,orderStat.refund_amount);

			orderStat.mch_id = channel_id;
			orderStat.trade_count = iterCl->second.total_count;
			orderStat.trade_amount = iterCl->second.total_fee;
			orderStat.trade_net_amount = orderStat.trade_amount - orderStat.refund_amount;
			orderStat.shop_net_amount = channel_profit;
			orderStat.shared_profit = channel_profit;

//			CDEBUG_LOG("out2 :orderStat.trade_amount=[%d],trade_net_amount=[%d],shop_net_amount=[%d]\n",
//								orderStat.trade_amount,orderStat.trade_net_amount,orderStat.shop_net_amount);

			//sum(total_fee)  交易金额
			//count(*) 交易笔数
			//sum(total_fee) - sum(refund_fee) 交易净额
			//channel_profit  sum(channel_profit) - 分润金额
			TRemitBill remitBill;
			remitBill.Reset();
			remitBill.account_id = channel_id;
			remitBill.sType = CHANNEL_TYPE_NAME;
			remitBill.sRemitfee = F2Y(toString(channel_profit));
			remitBill.remit_fee = channel_profit;
			remitBill.sPayTime = toDate(strPayTime);
			remitBill.sRemitTime = toDate(getSysDate());
			remitBill.sRemark = CHANNEL_REMARK_NAME;

			iRet = g_cOrderServer.CallGetBankNoApi(remitBill);
			if (iRet < 0)
			{
				CERROR_LOG("CallGetBankNoApi failed! iRet[%d].\n",iRet);
				throw(CTrsExp(ERR_NOTIFY_SETTLE_FAILED,"CallGetBankNoApi failed!"));
			}

			iRet = m_stTransFlowDao.InsertDistributionDB(*pBillDb,m_Req.sBmId,nowdate(m_Req.sInputTime),
									m_batch_no,m_Req.sPayChannel,channel_code,orderStat,remitBill,CHANNEL_TYPE_NAME);
			if (iRet < 0)
			{
				CERROR_LOG("InsertDistributionDB channel failed!iRet =[%d]", iRet);
				throw(CTrsExp(ERR_DB_UPDATE,"InsertDistributionDB channel failed!"));
			}


			if (channel_profit > 0)
			{
				//写结算表
				iRet = m_stTransFlowDao.InsertSettleDB(*pBillDb,m_Req.sBmId,nowdate(m_Req.sInputTime),
											m_batch_no,m_Req.sPayChannel,remitBill);
				if (iRet < 0)
				{
					CERROR_LOG("InsertSettleDB failed! iRet[%d].\n",iRet);
					throw(CTrsExp(ERR_DB_UPDATE,"InsertSettleDB failed!"));
				}
				//AddSettleLog(m_Req,remitBill,tremitMap,iIndex,total_amount);

			}
		}

		/**--------end---------------*/
		//清分技术服务方
		CDEBUG_LOG("total_service_profit:[%d] total_bm_profit[%d]\n", total_service_profit, total_bm_profit);
		TRemitBill remitBill;
		if (0 < total_service_profit)
		{


			remitBill.Reset();
			remitBill.account_id = m_Req.sBmId;
			remitBill.sType = SERVICE_TYPE_NAME;
			remitBill.sRemitfee = F2Y(toString(total_service_profit));
			remitBill.remit_fee = total_service_profit;
			remitBill.sPayTime = toDate(strPayTime);
			remitBill.sRemitTime = toDate(getSysDate());
			remitBill.sRemark = SERVICE_REMARK_NAME;

			//orderStat.Reset();
			//orderStat.trade_net_amount = total_service_profit;
			orderStat.mch_id = remitBill.account_id;
			orderStat.shared_profit  = total_service_profit;
			orderStat.shop_net_amount = total_service_profit;
			iRet = m_stTransFlowDao.InsertDistributionDB(*pBillDb,m_Req.sBmId,nowdate(m_Req.sInputTime),
									m_batch_no,m_Req.sPayChannel,channel_code,orderStat,remitBill,SERVICE_TYPE_NAME);
			if (iRet < 0)
			{
				CERROR_LOG("InsertDistributionDB service failed!iRet =[%d]", iRet);
				throw(CTrsExp(ERR_DB_UPDATE,"InsertDistributionDB service failed!"));
			}

			//写结算表
			iRet = m_stTransFlowDao.InsertSettleDB(*pBillDb,m_Req.sBmId,nowdate(m_Req.sInputTime),
										m_batch_no,m_Req.sPayChannel,remitBill);
			if (iRet < 0)
			{
				CERROR_LOG("InsertSettleDB failed! iRet[%d].\n",iRet);
				throw(CTrsExp(ERR_DB_UPDATE,"InsertSettleDB failed!"));
			}
			//AddSettleLog(m_Req,remitBill,tremitMap,iIndex,total_amount);

		}
		if (0 < total_bm_profit)
		{
			//清分银行机构方
			remitBill.Reset();
			remitBill.account_id = m_Req.sBmId;
			remitBill.sType = BM_TYPE_NAME;
			remitBill.sRemitfee = F2Y(toString(total_bm_profit));
			remitBill.remit_fee = total_bm_profit;
			remitBill.sPayTime = toDate(strPayTime);
			remitBill.sRemitTime = toDate(getSysDate());
			remitBill.sRemark = BM_REMARK_NAME;

			//orderStat.Reset();
			//orderStat.trade_net_amount = total_bm_profit;
			orderStat.mch_id  = remitBill.account_id;
			orderStat.shared_profit  = total_bm_profit;
			orderStat.shop_net_amount = total_bm_profit;

			iRet = m_stTransFlowDao.InsertDistributionDB(*pBillDb,m_Req.sBmId,nowdate(m_Req.sInputTime),
									m_batch_no,m_Req.sPayChannel,channel_code,orderStat,remitBill,BM_TYPE_NAME);
			if (iRet < 0)
			{
				CERROR_LOG("InsertDistributionDB bm failed!iRet =[%d]", iRet);
				throw(CTrsExp(ERR_DB_UPDATE,"InsertDistributionDB bm failed!"));
			}

				//写结算表
				iRet = m_stTransFlowDao.InsertSettleDB(*pBillDb,m_Req.sBmId,nowdate(m_Req.sInputTime),
											m_batch_no,m_Req.sPayChannel,remitBill);
				if (iRet < 0)
				{
					CERROR_LOG("InsertSettleDB failed! iRet[%d].\n",iRet);
					throw(CTrsExp(ERR_DB_UPDATE,"InsertSettleDB failed!"));
				}

			//AddSettleLog(m_Req,remitBill,tremitMap,iIndex,total_amount);

		}
		//m_total_count = tremitMap.size();
		//m_total_amount = total_amount;
		//提交事务
		m_mysql.Commit(*pBillDb);
	}

	catch(CTrsExp& e)
	{
		CERROR_LOG("transaction insert settle table fail:[%s]",e.retmsg.c_str());
		m_mysql.Rollback(*pBillDb);
		throw(CTrsExp(ERR_BILL_CONTRAST_MODE,e.retmsg));
	}


}

void CBillContrastSPDB::AddSettleLog(ProPullBillReq& m_Req,TRemitBill& remitBill,std::map<int, TRemitBill>& tremitMap,int& iIndex,int& total_amount)
{
	CDEBUG_LOG("AddSettleLog begin");
	int iRet;
	iRet = g_cOrderServer.CallGetBankNoApi(remitBill);
	if (iRet < 0)
	{
		CERROR_LOG("CallGetBankNoApi failed! iRet[%d].\n",iRet);
		throw(CTrsExp(ERR_NOTIFY_SETTLE_FAILED,"CallGetBankNoApi failed!"));
	}
	//写结算表
	iRet = m_stTransFlowDao.InsertSettleDB(*pBillDb,m_Req.sBmId,nowdate(m_Req.sInputTime),
								m_batch_no,m_Req.sPayChannel,remitBill);
	if (iRet < 0)
	{
		CERROR_LOG("InsertSettleDB failed! iRet[%d].\n",iRet);
		throw(CTrsExp(ERR_DB_UPDATE,"InsertSettleDB failed!"));
	}
}


void CBillContrastSPDB::UpdateBillStatus(ProPullBillReq& m_stReq,int starttime)
{
	CDEBUG_LOG("UpdateBillStatus begin");
	//更新对账批次表状态
	UpdateBillBatchDB(m_stReq);
}

/*
 *  子类独有函数
 */
std::string CBillContrastSPDB::GetPayChannelCode(ProPullBillReq& m_stReq)
{
	std::string mch_id;
	if(m_stReq.sPayChannel == WX_API_PAY_CHANNEL)
	{
		const CBillBusiConfig::WxPayCfg* p_wxpay_cfg = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig()
			->GetWxPayCfg(m_stReq.sBmId);
		if(NULL == p_wxpay_cfg)
		{
			CERROR_LOG("p_wxpay_cfg bussiness config not found");
			return "";
		}
		mch_id = p_wxpay_cfg->strMchId;
	}
	else if(m_stReq.sPayChannel == ALI_API_PAY_CHANNEL)
	{
		const CBillBusiConfig::AliPayCfg* p_alipay_cfg = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig()->
			GetAliPayCfg(m_stReq.sBmId);
		if(NULL == p_alipay_cfg)
		{
			CERROR_LOG("p_wxpay_cfg bussiness config not found");
			return "";
		}
		mch_id = p_alipay_cfg->strPid;
	}
	return mch_id;
}

void CBillContrastSPDB::UpdateDownLoadDB(ProPullBillReq& m_stReq,int bill_status,const char* file_name)
{
	int iRet = -1;
	clib_mysql* pBillDb = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
	CMySQL m_mysql;
	ostringstream sqlss;

    sqlss.str("");
    sqlss << "update "
    	  <<BILL_DB<<"."<<BILL_DOWNLOAD
		  <<" set bill_status = '"<<bill_status<<"',bill_filename = '"<< file_name <<"',modify_time = now()"
		  <<" where bm_id ='"<<m_stReq.sBmId<<"' and bill_date='"<<nowdate(m_stReq.sInputTime)
		  <<"' and pay_channel ='"<<m_stReq.sPayChannel<<"';";

    iRet = m_mysql.Execute(*pBillDb,sqlss.str().c_str());
    if(iRet != 1)
    {
    	CERROR_LOG("update bill_db.bill_download fail!!!");
    	throw(CTrsExp(ERR_DB_UPDATE,errMap[ERR_DB_UPDATE]));
    }
}

void CBillContrastSPDB::UpdateBillBatchDB(ProPullBillReq& m_stReq)
{
	int iRet = -1;
	clib_mysql* pBillDb = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
	CMySQL m_mysql;
	ostringstream sqlss;

    sqlss.str("");
    sqlss << "update "
    	  <<BILL_DB<<"."<<BILL_BATCH
		  <<" set bill_batch_status = '3',modify_time = now()"
		  <<" where bm_id ='"<<m_stReq.sBmId<<"' and bill_date='"<<nowdate(m_stReq.sInputTime)
		  <<"' and pay_channel ='"<<m_stReq.sPayChannel<<"' and bill_batch_no = '"<<m_batch_no<<"';";

    iRet = m_mysql.Execute(*pBillDb,sqlss.str().c_str());
    if(iRet != 1)
    {
    	CERROR_LOG("update bill_db.bill_download fail!!!");
    	throw(CTrsExp(ERR_DB_UPDATE,errMap[ERR_DB_UPDATE]));
    }
}

void CBillContrastSPDB::InitBillBatchDB(ProPullBillReq& m_stReq)
{
	int iRet = -1;
	clib_mysql* pBillDb = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
	CMySQL m_mysql;
	ostringstream sqlss;
	SqlResultSet outMap;
	SqlResultMapVector outmapVector;
	string seq_type;

    sqlss.str("");
    sqlss <<"select bill_date,pay_channel,bill_batch_status"
    	  <<" from "<<BILL_DB<<"."<<BILL_BATCH
		  <<" where bm_id ='"<<m_stReq.sBmId<<"' and bill_date='"<<nowdate(m_stReq.sInputTime)
		  <<"' and pay_channel ='"<<m_stReq.sPayChannel<<"';";

    iRet = m_mysql.QryAndFetchResMVector(*pBillDb,sqlss.str().c_str(),outmapVector);
    if(iRet == 1)
    {
    	for(unsigned int i = 0; i < outmapVector.size(); ++i)
    	{
    		if(outmapVector[i]["bill_batch_status"] == "2")
    		{
        		CERROR_LOG("Reconciliation is running !!!");
        		throw(CTrsExp(ERR_BILL_BATCH_EXIST,"Reconciliation is running!!"));
    		}
    	}

    }

    //检验是否有结算单记录
    sqlss.str("");
    sqlss <<"select batch_no from "<<BILL_DB<<"."<<BILL_SETTLE
    	  <<" where bm_id = '"<<m_stReq.sBmId<<"' and bill_date='"<<nowdate(m_stReq.sInputTime)
		  <<"' and pay_channel = '"<<m_stReq.sPayChannel<<"';";
    iRet = m_mysql.QryAndFetchResMap(*pBillDb,sqlss.str().c_str(),outMap);
    if(iRet == 1)
    {
		CERROR_LOG("settle record has exist [%] !!!",outMap["batch_no"].c_str());
		throw(CTrsExp(ERR_BILL_BATCH_EXIST,errMap[ERR_BILL_FILE_EXIST]));
    }


	if(m_stReq.sPayChannel == WX_API_PAY_CHANNEL)
	{
		seq_type = "wx";
	}
	if(m_stReq.sPayChannel == ALI_API_PAY_CHANNEL)
	{
		seq_type = "zfb";
	}

	m_batch_no = "";
	GetBatchNo(seq_type,nowdate(m_stReq.sInputTime),m_batch_no);

	//写对账批次表
    sqlss.str("");
    sqlss <<"insert into "
    	  <<BILL_DB<<"."<<BILL_BATCH
		  <<" (bm_id,bill_batch_no,bill_date,pay_channel,bank_inscode,bill_batch_status,operator_id)"
		  <<" values('"<<m_stReq.sBmId<<"','"<<m_batch_no<<"','"<<nowdate(m_stReq.sInputTime)<<"','"<<m_stReq.sPayChannel
		  <<"','"<<GetPayChannelCode(m_stReq)<<"','1','"<<m_stReq.sOperator<<"');";

    iRet = m_mysql.Execute(*pBillDb,sqlss.str().c_str());
    if(iRet != 1)
    {
    	CERROR_LOG("insert bill_db.bill_batch_manage fail!!!");
    	throw(CTrsExp(ERR_DB_INSERT,"insert bill_db.bill_batch_manage fail!!"));
    }

}

void CBillContrastSPDB::GetBatchNo(const string& strtype,const string& bill_date,string& seq_no)
{
	CDEBUG_LOG("GetBatchNo begin....");
	//int iRet = -1;
	//char szToBuff[1024] = { 0 };
	//seq_no = "";
	char szRecvBuff[1024] = {0};
	StringMap paramMap;
	StringMap recvMap;
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;
	CSocket* idSocket = Singleton<CSpeedPosConfig>::GetInstance()->GetIDServerSocket();

	paramMap.insert(StringMap::value_type("cmd", "4"));
	paramMap.insert(StringMap::value_type("key",strtype));  //zfb
	paramMap.insert(StringMap::value_type("iIdLen","3"));
	//MapToUrl(paramMap,szToBuff,sizeof(szToBuff),"\r\n");

	idSocket->SendAndRecvLineEx(paramMap,szRecvBuff,sizeof(szRecvBuff),"\r\n");
	CDEBUG_LOG("recv Msg [%s]",szRecvBuff);

	if(NULL == szRecvBuff||strlen(szRecvBuff) == 0)  //
	{
		CDEBUG_LOG("get BatchNo Fail!!,err_msg[%s]",recvMap["retmsg"].c_str());
		throw(CTrsExp(SYSTEM_ERR,"get BatchNo Fail!!"));
	}

	Kv2Map(szRecvBuff,recvMap);

	CDEBUG_LOG("call GetBatchNo success,retcode=[%s],order_no=[%s]!",recvMap["retcode"].c_str(),recvMap["order_no"].c_str());
	if(recvMap["retcode"] != "0")
	{
		CDEBUG_LOG("get BatchNo Fail!!,err_msg[%s]",recvMap["retmsg"].c_str());
		throw(CTrsExp(SYSTEM_ERR,"get BatchNo Fail!!"));
	}

	if(strtype == "wx")
	{
		seq_no += "WX" + bill_date + strTrimSpecial(recvMap["order_no"]);
	}
	if(strtype == "zfb")
	{
		seq_no += "ALI" + bill_date + strTrimSpecial(recvMap["order_no"]);
	}

	CDEBUG_LOG("get batch no [%s]",seq_no.c_str());


//	StringMap recvMap;
//	CSocket socket;
//	socket.SetIP(mainConfig.sCallSettleSerIp.c_str());
//	socket.SetPort(mainConfig.iCallSettleSerPort);
//	socket.SendAndRecvLine(paramMap,recvMap,"\r\n");
//
//	MapToUrl(recvMap,szToBuff,sizeof(szToBuff),"");
//	CDEBUG_LOG("recv [%s]",szToBuff);


//	int socket_fd = safe_tcp_connect_timeout(mainConfig.sCallSettleSerIp.c_str(), mainConfig.iCallSettleSerPort, 30);
//	if (socket_fd)
//	{
//		iRet = safe_tcp_send_n(socket_fd,szToBuff,sizeof(szToBuff));
//		if(iRet > 0)
//		{
//			safe_tcp_recv(socket_fd,szRecvBuff,sizeof(szRecvBuff));
//			CDEBUG_LOG("recv retmsg = [%s]",szRecvBuff);
////			UrlParamMap reqMap;
////			reqMap.parseUrl(szRecvBuff);
////			UrlParamMap::iterator iterUrlMap;
////			iterUrlMap = reqMap.find("retcode");
////			if (iterUrlMap != reqMap.end())
////			{
////				if(atoi(iterUrlMap->second.c_str()) == 0)
////				{
////
////				}
////
////			}
//		}
//	}
}

void CBillContrastSPDB::ProcOverFlowData(const string& resBody,const string& tableName,ProPullBillReq& m_stReq)
{
	int iRet;
	tinyxml2::XMLDocument rsp_doc;
	if (tinyxml2::XML_SUCCESS != rsp_doc.Parse(resBody.c_str(), resBody.size()))
	{
		CERROR_LOG("Parse XML format error!!");
		throw CTrsExp(ERR_PASE_RETRUN_DATA,"Error:Parse XML format error!!");
	}
	tinyxml2::XMLElement * xmlElement = rsp_doc.FirstChildElement("xml");
	if (NULL == xmlElement)
	{
		CERROR_LOG("Find XML node:[xml] error!");
		throw CTrsExp(ERR_PASE_RETRUN_DATA,"Error:Find XML node:[xml] error!!");
	}
	const char* ret_code = g_cOrderServer.GetXmlField(xmlElement, "retcode");
	//const char* ret_msg = g_cOrderServer.GetXmlField(xmlElement, "retmsg");
	if(strcmp(ret_code,"0") == 0)  //有返回，应该是状态不一致
	{
		//根据接口返回的字段添加success表，t_order_channel 表，并更新异常表异常状态和处理状态
		StringMap orderMap;
		const char* ret_content = g_cOrderServer.GetXmlField(xmlElement, "content");
		cJSON* root = cJSON_Parse(ret_content);

		orderMap["bm_id"]  				= cJSON_GetObjectItem(root, "bm_id")->valuestring;
		orderMap["bm_profit"]  			= ITOS(cJSON_GetObjectItem(root, "bm_profit")->valueint);
		orderMap["channel_id"]  		= cJSON_GetObjectItem(root, "channel_id")->valuestring;
		orderMap["channel_profit"]  	= ITOS(cJSON_GetObjectItem(root, "channel_profit")->valueint);
		orderMap["mch_id"]  			= cJSON_GetObjectItem(root, "mch_id")->valuestring;
		orderMap["order_no"]  			= cJSON_GetObjectItem(root, "order_no")->valuestring;
		orderMap["order_status"]  		= cJSON_GetObjectItem(root, "order_status")->valuestring;
		orderMap["out_order_no"]  		= cJSON_GetObjectItem(root, "out_order_no")->valuestring;
		orderMap["out_refund_no"]  		= cJSON_GetObjectItem(root, "out_refund_no")->valuestring;
		orderMap["pay_channel"]  		= cJSON_GetObjectItem(root, "pay_channel")->valuestring;
		orderMap["pay_time"]  			= cJSON_GetObjectItem(root, "pay_time")->valuestring;
		orderMap["payment_profit"]  	= ITOS(cJSON_GetObjectItem(root, "payment_profit")->valueint);
		orderMap["refund_fee"]  		= ITOS(cJSON_GetObjectItem(root, "refund_fee")->valueint);
		orderMap["refund_id"]  			= cJSON_GetObjectItem(root, "refund_id")->valuestring;
		orderMap["refund_no"]  			= cJSON_GetObjectItem(root, "refund_no")->valuestring;
		orderMap["service_profit"]  	= ITOS(cJSON_GetObjectItem(root, "service_profit")->valueint);
		orderMap["shop_amount"]  		= ITOS(cJSON_GetObjectItem(root, "shop_amount")->valueint);
		orderMap["total_commission"]	= ITOS(cJSON_GetObjectItem(root, "total_commission")->valueint);
		orderMap["total_fee"]  			= ITOS(cJSON_GetObjectItem(root, "total_fee")->valueint);
		orderMap["trade_type"]  		= cJSON_GetObjectItem(root, "trade_type")->valuestring;
		orderMap["transaction_id"]  	= cJSON_GetObjectItem(root, "transaction_id")->valuestring;

		//校验银行编号和渠道
		if(orderMap["bm_id"] != m_stReq.sBmId || orderMap["pay_channel"] != m_stReq.sPayChannel)
		{
			CERROR_LOG("ret_msg bm_id or channel not match bm_id：[%s]--[%s],channel:[%s]---[%s]!",
					orderMap["bm_id"].c_str(),m_stReq.sBmId.c_str(),
					orderMap["pay_channel"].c_str(),m_stReq.sPayChannel.c_str());
			return;
		}

		//嵌套Json
		cJSON * chanel_detail = cJSON_GetObjectItem(root, "chanel_detail");
		int iTotal = cJSON_GetArraySize(chanel_detail);
		CDEBUG_LOG("channel list [%d]",iTotal);
		for (int i = 0; i < iTotal; ++i)
		{
			cJSON* channel = cJSON_GetArrayItem(chanel_detail, i);
			JsonType obj = JsonUtil::json2obj(channel);
			JsonMap jMap = obj.toMap();

			StringMap channelMap;

			channelMap["bm_id"] 				= jMap["bm_id"].toString();
			channelMap["channel_id"] 			= jMap["channel_id"].toString();
			channelMap["channel_profit"] 		= LTOS(jMap["channel_profit"].toNumber());
			channelMap["channel_profit_rate"] 	= LTOS(jMap["channel_profit_rate"].toNumber());
			channelMap["mch_id"] 				= jMap["mch_id"].toString();
			channelMap["order_no"] 				= jMap["order_no"].toString();
			channelMap["order_status"] 			= jMap["order_status"].toString();
			channelMap["pay_channel"] 			= jMap["pay_channel"].toString();
			channelMap["pay_time"] 				= jMap["pay_time"].toString();
			channelMap["refund_fee"] 			= LTOS(jMap["refund_fee"].toNumber());
			channelMap["refund_no"] 			= jMap["refund_no"].toString();
			channelMap["total_fee"] 			= LTOS(jMap["total_fee"].toNumber());
			channelMap["trade_type"] 			= jMap["trade_type"].toString();

			CDEBUG_LOG("channel_profit_rate = %s",channelMap["channel_profit_rate"].c_str());
			//补渠道表
			iRet = m_stTransFlowDao.InsertToChannelFlow(*pBillDb,tableName,channelMap);
			if (iRet < 0)
			{
				CERROR_LOG("InsertToChannelFlow failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
				throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
			}
		}
		//补订单表
		iRet = m_stTransFlowDao.InsertTOOrderSuccFlow(*pBillDb, orderMap);
		if (iRet < 0)
		{
			CERROR_LOG("InsertTOOrderSuccFlow failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
		//更新异常表状态
		iRet = m_stTransFlowDao.UpdateAbnormalStatus(*pBillDb,orderMap);
		if (iRet < 0)
		{
			CERROR_LOG("UpdateAbnormalStatus failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
	}

}

void CBillContrastSPDB::ProcException(ProPullBillReq& m_stReq,int starttime,int exce_type)
{
	if(exce_type == 1)
	{
		if(m_batch_no.empty()) //批次号没生成或为空时，不更新记录
		{
			return ;
		}

		int iRet = -1;
		clib_mysql* pBillDb = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
		CMySQL m_mysql;
		ostringstream sqlss;

	    sqlss.str("");
	    sqlss << "update "
	    	  <<BILL_DB<<"."<<BILL_BATCH
			  <<" set bill_batch_status = '4',modify_time = now()"
			  <<" where bm_id ='"<<m_stReq.sBmId<<"' and bill_date='"<<nowdate(m_stReq.sInputTime)
			  <<"' and pay_channel ='"<<m_stReq.sPayChannel<<"' and bill_batch_no = '"<<m_batch_no<<"';";

	    iRet = m_mysql.Execute(*pBillDb,sqlss.str().c_str());
	    if(iRet != 1)
	    {
	    	CERROR_LOG("update bill_db.bill_download fail!!!");
	    	throw(CTrsExp(ERR_DB_UPDATE,errMap[ERR_DB_UPDATE]));
	    }


	}

}


