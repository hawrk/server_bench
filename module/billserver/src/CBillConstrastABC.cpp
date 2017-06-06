/*
 * CBillContrastABC.cpp
 *
 *  Created on: 2017年6月2日
 *      Author: hawrkchen
 */
#include "CBillConstrastABC.h"

extern CSpeedPosServer g_cOrderServer;

CBillContrastABC::CBillContrastABC()
{
	//TODO:
	CDEBUG_LOG("CBillContrastABC begin");
	pBillDb = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
}

CBillContrastABC::~CBillContrastABC()
{
	CDEBUG_LOG("~CBillContrastABC begin");
	//TODO:
//	if(NULL != pBillDb)
//	{
//		delete pBillDb;
//		pBillDb = NULL;
//	}
}


void CBillContrastABC::BillFileDownLoad(ProPullBillReq& m_Req,int starttime)
{
	SFTPDownLoad(m_Req,starttime);
}

void CBillContrastABC::LoadBillFlowToDB(ProPullBillReq& m_stReq,int starttime)
{
	//for sybase
	BEGIN_LOG(__func__);
	CDEBUG_LOG("LoadBillFlowToDB begin");
	INT32 iRet = 0;
	//同步对账步骤
	//TODO
//	iRet = g_cOrderServer.CallUpdateBillContrastApi(m_stReq.sBmId, m_stReq.sPayChannel, toDate(getSysDate(starttime)), BILL_CONTRAST_STEP_BEGIN_LOAD_BILL_FLOW,
//					stepMap[BILL_CONTRAST_STEP_BEGIN_LOAD_BILL_FLOW]);
//	if (iRet < 0)
//	{
//		CERROR_LOG("CallAddBillContrastApi failed!iRet =[%d]", iRet);
//		throw(CTrsExp(ERR_ORDER_NO_MAPPING_EXIST,errMap[ERR_ORDER_NO_MAPPING_EXIST]));
//	}
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;
	std::string strDbName = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDbName();
	//char* path_end;
//	char dir[256] = { 0 };
//	//int n = readlink("/proc/self/exe", dir, 256);
//	readlink("/proc/self/exe",dir,256);
//	std::string sDir = dir;
//	int nLen = sDir.rfind('/');
//	std::string sPath = sDir.substr(0, nLen + 1);
	std::string sPath = mainConfig.sSftpShellPath;
	CDEBUG_LOG("sPath : [%s] \n", sPath.c_str());

	//先清表
	string pay_channel;
	if(m_stReq.sPayChannel == WX_API_PAY_CHANNEL)
	{
		pay_channel = "wx";
	}
	else if(m_stReq.sPayChannel == ALI_API_PAY_CHANNEL)
	{
		pay_channel = "ali";
	}
	m_stTransFlowDao.TruncateEveryPaymentTypeSysFlowData(*pBillDb,m_stReq.sBmId,pay_channel);

	char szLoadOrderFlowCmd[512] = { 0 };
	snprintf(szLoadOrderFlowCmd, sizeof(szLoadOrderFlowCmd), "sh %s%s %s %s %s %s %s t_order_all_flow_%s shop",
		sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(), pBillDb->ms_host,
		pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(),sShopBillSrcFileName.c_str(), m_stReq.sBmId.c_str());
	CDEBUG_LOG("szLoadOrderFlowCmd = [%s]",szLoadOrderFlowCmd);
	system(szLoadOrderFlowCmd);

	char szLoadOrderChannelFlowCmd[512] = { 0 };
	snprintf(szLoadOrderChannelFlowCmd, sizeof(szLoadOrderChannelFlowCmd), "sh %s%s %s %s %s %s %s t_order_channel_all_flow_%s channel",
		sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(), pBillDb->ms_host,
		pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(), sChannelBillSrcFileName.c_str(), m_stReq.sBmId.c_str());
	CDEBUG_LOG("LoadOrderChannelFlowCmd:[%s] \n", szLoadOrderChannelFlowCmd);
	system(szLoadOrderChannelFlowCmd);

	if (0 == strcmp(m_stReq.sPayChannel.c_str(), WX_API_PAY_CHANNEL))
	{
		char szLoadWxFlowCmd[512] = { 0 };
		snprintf(szLoadWxFlowCmd, sizeof(szLoadWxFlowCmd), "sh %s%s %s %s %s %s %s t_wxpay_flow_%s wx",
			sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(), pBillDb->ms_host,
			pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(), sWxBillSrcFileName.c_str(), m_stReq.sBmId.c_str());
		CDEBUG_LOG("LoadWxFlowCmd:[%s] \n", szLoadWxFlowCmd);
		system(szLoadWxFlowCmd);
	}
	if (0 == strcmp(m_stReq.sPayChannel.c_str(), ALI_API_PAY_CHANNEL))
	{
		char szLoadAliFlowCmd[512] = { 0 };
		snprintf(szLoadAliFlowCmd, sizeof(szLoadAliFlowCmd), "sh %s%s %s %s %s %s %s t_alipay_flow_%s ali",
			sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(), pBillDb->ms_host,
			pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(), sAliBillSrcFileName.c_str(), m_stReq.sBmId.c_str());
		CDEBUG_LOG("szLoadAliFlowCmd:[%s] \n", szLoadAliFlowCmd);
		system(szLoadAliFlowCmd);
	}
//	std::string strWxOrderSql = "t_order_wxpay_flow_" + m_stReq.sBmId;
//	std::string strAliOrderSql = "t_order_alipay_flow_" + m_stReq.sBmId;
//	std::string strWxOrderChannelSql = "t_order_channel_wxpay_flow_" + m_stReq.sBmId;
//	std::string strAliOrderChannelSql = "t_order_channel_alipay_flow_" + m_stReq.sBmId;
//	char szTruncateDBCmd[512] = { 0 };
//	snprintf(szTruncateDBCmd, sizeof(szTruncateDBCmd), "sh %s%s %s %s %s %s %s %s %s %s",
//		sPath.c_str(), mainConfig.sBillTruncateDBShellPath.c_str(), pBillDb->ms_host,
//		pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(), strWxOrderSql.c_str(), strAliOrderSql.c_str(), strWxOrderChannelSql.c_str(), strAliOrderChannelSql.c_str());
//	CDEBUG_LOG("szTruncateDBCmd:[%s] \n", szTruncateDBCmd);
//	system(szTruncateDBCmd);

}

void CBillContrastABC::GetRemitBillData(ProPullBillReq& m_stReq,int starttime,int endtime)
{
	CDEBUG_LOG("GetRemitBillData begin");
	INT32 iRet = 0;
	std::string sBeginTime = getSysTime(starttime);
	std::string sEndTime = getSysTime(endtime);

	iRet = m_stTransFlowDao.GetPayBillData(*pBillDb,
			m_stReq.sBmId,
		sBeginTime,
		sEndTime,
		orderPayBillMap);
	if (iRet < 0)
	{
		CERROR_LOG("GetPayBillData failed!iRet =[%d]", iRet);
		throw(CTrsExp(ERR_QUERY_RECORD_ERR,errMap[ERR_QUERY_RECORD_ERR]));
	}


	iRet = m_stTransFlowDao.GetRefundBillData(*pBillDb,
			m_stReq.sBmId,
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
		strTableFix = "t_order_channel_wxpay_flow_" + m_stReq.sBmId;
	}
	else if (m_stReq.sPayChannel == ALI_API_PAY_CHANNEL)
	{
		strTableFix = "t_order_channel_alipay_flow_" + m_stReq.sBmId;
	}

	iRet = m_stTransFlowDao.GetChannelBillData(*pBillDb,
		strTableFix,
		m_stReq.sBmId,
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

void CBillContrastABC::ProcBillComparison(ProPullBillReq& m_stReq,int starttime,int endtime)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	CDEBUG_LOG("ProcBillComparison: Begin\n");
	//TODO
//	iRet = g_cOrderServer.CallUpdateBillContrastApi(m_stReq.sBmId, m_stReq.sPayChannel, toDate(getSysDate(starttime)), BILL_CONTRAST_STEP_BEGIN_RECONCILIATION,
//			   stepMap[BILL_CONTRAST_STEP_BEGIN_RECONCILIATION]);
//	if (iRet < 0)
//	{
//		CERROR_LOG("CallAddBillContrastApi failed!iRet =[%d]", iRet);
//		throw(CTrsExp(ERR_ORDER_NO_MAPPING_EXIST,errMap[ERR_ORDER_NO_MAPPING_EXIST]));
//	}
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
	}
	/****-------------------------------------------------------------------***/

	/*** ---------------找出支付宝支付 相同的数据和不同数据---------------------------**/
	if (0 == strcmp(m_stReq.sPayChannel.c_str(), ALI_API_PAY_CHANNEL))
	{
		iRet = m_stTransFlowDao.InsertAliPayIdenticalToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime,"SUCCESS");
		if (iRet < 0)
		{
			CERROR_LOG("InsertAliPayIdenticalToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}

		iRet = m_stTransFlowDao.InsertAliPayIdenticalRefundToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime);
		if (iRet < 0)
		{
			CERROR_LOG("InsertAliPayIdenticalRefundToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}

		iRet = m_stTransFlowDao.InsertAliPayDistinctToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime,"SUCCESS");
		if (iRet < 0)
		{
			CERROR_LOG("InsertAliPayDistinctToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}

		iRet = m_stTransFlowDao.InsertAliPayDistinctRefundToDB(*pBillDb,m_stReq.sBmId,sBeginTime,sEndTime);
		if (iRet < 0)
		{
			CERROR_LOG("InsertAliPayDistinctRefundToDB failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
	}
	/****-------------------------------------------------------------------***/

	//check wx is overflow
	if (m_stReq.sPayChannel == WX_API_PAY_CHANNEL)
	{
		std::vector<WxFlowSummary> wxOverFlowVec;
		iRet = m_stTransFlowDao.GetWxOverFlowData(*pBillDb,
				m_stReq.sBmId,
			sBeginTime,
			sEndTime,
			wxOverFlowVec);
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
				SOrderInfoRsp orderInfo;
				orderInfo.Rest();
				CDEBUG_LOG("Wxpay OverFlow order_no:[%s]\n", wxOverFlowVec[iIndex].order_no.c_str());
				iRet = g_cOrderServer.sppClent.CallOrderQuery(wxOverFlowVec[iIndex].order_no, orderInfo);
				int problem_type = 3;  //状态不一致
				if (iRet < 0)
				{
					problem_type = 2;  //支付渠道多
				}
				g_cOrderServer.CallWxpayExceptionOrderMsgApi(m_stReq.sBmId, problem_type, wxOverFlowVec[iIndex]);
			}
			throw(CTrsExp(ERR_WXPAY_NOT_RECONCLIED,errMap[ERR_WXPAY_NOT_RECONCLIED]));
		}
	}
	if (m_stReq.sPayChannel ==  ALI_API_PAY_CHANNEL)
	{
		std::vector<AliFlowSummary> aliOverFlowVec;
		iRet = m_stTransFlowDao.GetAliOverFlowData(*pBillDb,
				m_stReq.sBmId,
			sBeginTime,
			sEndTime,
			aliOverFlowVec);
		if (iRet < 0)
		{
			CERROR_LOG("GetAliOverFlowData failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage());
		}
		CDEBUG_LOG("aliOverFlowVec [%d]\n", aliOverFlowVec.size());
		if (aliOverFlowVec.size())//检测有溢出的数据
		{
			for (std::vector<AliFlowSummary>::iterator iter = aliOverFlowVec.begin(); iter != aliOverFlowVec.end(); ++iter)
			{
				CDEBUG_LOG("Alipay OverFlow order_no:[%s]\n", iter->order_no.c_str());
				SOrderInfoRsp orderInfo;
				orderInfo.Rest();
				//CDEBUG_LOG("Alipay OverFlow order_no:[%d]\n", aliOverFlowVec[iIndex].order_no);
				iRet = g_cOrderServer.sppClent.CallOrderQuery(iter->order_no, orderInfo);
				int problem_type = 3;
				if (iRet < 0)
				{
					problem_type = 2;
				}
				g_cOrderServer.CallAlipayExceptionOrderMsgApi(m_stReq.sBmId, problem_type, *iter);
			}
			throw(CTrsExp(ERR_ALIPAY_NOT_RECONCLIED,errMap[ERR_ALIPAY_NOT_RECONCLIED]));
		}
	}
}

void CBillContrastABC::UpdateBillStatus(ProPullBillReq& m_stReq,int starttime)
{
	int iRet;
	CDEBUG_LOG("UpdateBillStatus begin");
	std::string strPayTime = getSysDate(starttime);
	//const CBillBusiConfig::BankAttr* p_bank_attr = pBillBusConfig->GetBankAttrCfg(m_stReq.sBmId);
	//发送结算单通知
	if(tremitMap.size() > 0)
	{
		iRet = g_cOrderServer.CallUpdateBillContrastApi(m_stReq.sBmId, m_stReq.sPayChannel, toDate(strPayTime), BILL_CONTRAST_STEP_BEGIN_GENERATE_STATEMENT,
			stepMap[BILL_CONTRAST_STEP_BEGIN_GENERATE_STATEMENT]);
		if (iRet < 0)
		{
			CERROR_LOG("CallUpdateBillContrastApi failed!iRet =[%d]", iRet);
			throw(CTrsExp(ERR_UPDATE_BILL_STATUS,errMap[ERR_UPDATE_BILL_STATUS]));
		}

//		//生成加密文件
//		if(p_bank_attr->strEncrypt == "1")  //需要加密
//		{
		CDEBUG_LOG("encrypt file name:[%s],cipher file name=[%s]",
				pSettleFile->GetFileName().c_str(),pSettleCipherFile->GetFileName().c_str());
		DES_Encrypt(pSettleFile->GetFileName().c_str(),pSettleCipherFile->GetFileName().c_str());
//		}
//		else
//		{
//			tars::TC_File::copyFile(pSettleFile->GetFileName(),pSettleCipherFile->GetFileName(),true);
//			//CopyFile(pSettleFile->GetFileName().c_str(),pSettleCipherFile->GetFileName().c_str()); //覆盖
//		}
		iRet = NotifySettleServer(m_stReq);
		if(iRet != 0)
		{
			CERROR_LOG("NotifySettledServer failed!iRet =[%d]", iRet);
			throw(CTrsExp(ERR_NOTIFY_SETTLE_FAILED,errMap[ERR_NOTIFY_SETTLE_FAILED]));
		}

		iRet = g_cOrderServer.CallUpdateBillContrastApi(m_stReq.sBmId, m_stReq.sPayChannel, toDate(strPayTime), BILL_CONTRAST_STEP_BEGIN_NOTIFY_INSTITUTION,
			stepMap[BILL_CONTRAST_STEP_BEGIN_NOTIFY_INSTITUTION], 1, m_total_count, m_total_amount);
		if (iRet < 0)
		{
			CERROR_LOG("CallUpdateBillContrastApi failed!iRet =[%d]", iRet);
			throw(CTrsExp(ERR_UPDATE_BILL_STATUS,errMap[ERR_UPDATE_BILL_STATUS]));
		}

	}
	else
	{
		iRet = g_cOrderServer.CallUpdateBillContrastApi(m_stReq.sBmId, m_stReq.sPayChannel, toDate(strPayTime), BILL_CONTRAST_STEP_BEGIN_GENERATE_STATEMENT,
			stepMap[BILL_CONTRAST_STEP_BEGIN_GENERATE_STATEMENT], 1);
		if (iRet < 0)
		{
			CERROR_LOG("CallAddBillContrastApi failed!iRet =[%d]", iRet);
			throw(CTrsExp(ERR_UPDATE_BILL_STATUS,errMap[ERR_UPDATE_BILL_STATUS]));
		}
	}
}

void CBillContrastABC::AddSettleLog(TRemitBill& remitBill,std::map<int, TRemitBill>& tremitMap,int& iIndex,int& total_amount)
{
	total_amount += remitBill.remit_fee;
	tremitMap.insert(std::make_pair(iIndex, remitBill));
	++iIndex;
}

INT32 CBillContrastABC::NotifySettleServer(ProPullBillReq& m_stReq)
{
	CDEBUG_LOG("notify to settle server");
	int iRet = -1;
	char szToBuff[1024] = { 0 };
	char szRecvBuff[1024] = {0};
	StringMap paramMap;
	const CBillBusiConfig::BankAttr* p_bank_attr = pBillBusConfig->GetBankAttrCfg(m_stReq.sBmId);
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;

	string bill_date = pSettleFile->GetFileName().substr(pSettleFile->GetFileName().find("ZF")+2,8);

	if (!p_bank_attr->strBankType.empty())
	paramMap.insert(StringMap::value_type("bkname", p_bank_attr->strBankType));
	paramMap.insert(StringMap::value_type("ops","smb"));
	paramMap.insert(StringMap::value_type("bmid",m_stReq.sBmId));
	paramMap.insert(StringMap::value_type("paychannel",m_stReq.sPayChannel));
	paramMap.insert(StringMap::value_type("billdate",bill_date));
	MapToUrl(paramMap,szToBuff,sizeof(szToBuff),"\r\n");

	CDEBUG_LOG("send to:IP:[%s],port:[%d],Msg:[%s]",mainConfig.sCallSettleSerIp.c_str(),mainConfig.iCallSettleSerPort,szToBuff);
	//iRet = settleClient.SendBillNotify(paramMap,orderInfo);
	int socket_fd = safe_tcp_connect_timeout(mainConfig.sCallSettleSerIp.c_str(), mainConfig.iCallSettleSerPort, 30);
	if (socket_fd)
	{
		iRet = safe_tcp_send_n(socket_fd,szToBuff,sizeof(szToBuff));
		if(iRet > 0)
		{
			safe_tcp_recv(socket_fd,szRecvBuff,sizeof(szRecvBuff));
			CDEBUG_LOG("recv retmsg = [%s]",szRecvBuff);
			UrlParamMap reqMap;
			reqMap.parseUrl(szRecvBuff);
			UrlParamMap::iterator iterUrlMap;
			iterUrlMap = reqMap.find("ret_code");
			if (iterUrlMap != reqMap.end())
			{
				iRet = atoi(iterUrlMap->second.c_str());
			}
		}
	}
	return iRet;
}

