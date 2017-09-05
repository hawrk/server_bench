/*
 * CBillconstrstBase.cpp
 *
 *  Created on: 2017年6月2日
 *      Author: hawrkchen
 */

#include "CBillConstrastBase.h"
#include "CCommFunc.h"

extern CSpeedPosServer g_cOrderServer;


CBillContrastBase::CBillContrastBase()
{
	CDEBUG_LOG("CBillContrastBase begin");
	sShopBillSrcFileName = "";
	sShopBillDecFileName = "";
	sChannelBillSrcFileName = "";
	sChannelBillDecFileName = "";
	sWxBillSrcFileName = "";
	sWxBillDecFileName = "";
	sAliBillSrcFileName = "";
	sAliBillDecFileName = "";

	pBillChannelFille = NULL;
	pBillWxFile = NULL;
	pBillAliFile = NULL;
	pBillShopFille = NULL;
	pSettleFile = NULL;
	pSettleCipherFile = NULL;
	pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();

	m_total_count = 0;
	m_total_amount = 0;
}

CBillContrastBase::~CBillContrastBase()
{
	CDEBUG_LOG("~CBillContrastBase begin");
	if (pBillWxFile)
	{
		delete pBillWxFile;
		pBillWxFile = NULL;
	}
	if (pBillAliFile)
	{
		delete pBillAliFile;
		pBillAliFile = NULL;
	}
	if (pBillShopFille)
	{
		delete pBillShopFille;
		pBillShopFille = NULL;
	}
	if (pBillChannelFille)
	{
		delete pBillChannelFille;
		pBillChannelFille = NULL;
	}
	if (pSettleFile)
    {
		delete pSettleFile;
		pSettleFile = NULL;
    }
	if(pSettleCipherFile)
	{
		delete pSettleCipherFile;
		pSettleCipherFile = NULL;
	}
	//pBillBusConfig = NULL;
}

void CBillContrastBase::BillFileDownLoad(ProPullBillReq& m_Req,int starttime)
{
	//do nothing
}

void CBillContrastBase::LoadBillFlowToDB(ProPullBillReq& m_stReq,int starttime)
{
	//do nothing
}


void CBillContrastBase::ProcBillComparison(ProPullBillReq& m_stReq,int starttime,int endtime)
{
	//do nothing
}

void CBillContrastBase::GetRemitBillData(ProPullBillReq& m_Req,int starttime,int endtime)
{
	//do nothing
}

void CBillContrastBase::UpdateBillStatus(ProPullBillReq& m_stReq,int starttime)
{
	//do nothing
}

INT32 CBillContrastBase::NotifySettleServer(ProPullBillReq& m_stReq)
{
	//do nothing
	return 0;
}

void CBillContrastBase::InitBillFileDownLoad(ProPullBillReq& m_Req,int starttime)
{
	//do nothing
}

void CBillContrastBase::ProcException(ProPullBillReq& m_stReq,int starttime,int exce_type)
{
	//do nothing
}

void CBillContrastBase::CheckBillFill(ProPullBillReq& m_Req,int starttime)
{
	BEGIN_LOG(__func__);
	CDEBUG_LOG("CheckBillFill begin");
	INT32 iRet = 0;

	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;
	const CBillBusiConfig::BankAttr* p_bank_attr = pBillBusConfig->GetBankAttrCfg(m_Req.sBmId);
	if (!p_bank_attr)
	{
		CERROR_LOG("BmId [%s] has no config!!!.\n",m_Req.sBmId.c_str());
		throw CTrsExp(ERR_CONFIG_NOT_FOUND,errMap[ERR_CONFIG_NOT_FOUND]);
	}

	///paybill/1001/src
	std::string strPath = mainConfig.sRemittancePath + m_Req.sBmId + "/";
	CDEBUG_LOG("strPath:[%s] \n", strPath.c_str());
	//判断银行编号目录是否存在
	if(!tars::TC_File::makeDir(strPath))
	{
		CERROR_LOG("  create dir fail! strPath[%s] Err.\n", strPath.c_str());
	}

	std::string strsrcPath = strPath + SRC_PATH;         //原文件目录
	std::string strEncryptPath = strPath + ENCRYPT_PATH;  //结算加密后目录
	std::string strResultPath = strPath + "result/";   //顺便创建结算结果的目录

	if(!tars::TC_File::makeDir(strsrcPath)
	 || !tars::TC_File::makeDir(strEncryptPath)
	 || !tars::TC_File::makeDir(strResultPath))
	{
		CERROR_LOG("  create dir fail! strPath[%s] Err.\n", strsrcPath.c_str());
	}

	std::string strFileName, strCipherFileName;
	if (0 == strcmp(m_Req.sPayChannel.c_str(), WX_API_PAY_CHANNEL))
	{
		strFileName = p_bank_attr->strBankType + "_WXPAYZF" + getSysDate(starttime) + ".csv";
		strCipherFileName = p_bank_attr->strBankType + "_WXPAYZF" + getSysDate(starttime) + ".SRC";
	}
	else if (0 == strcmp(m_Req.sPayChannel.c_str(), ALI_API_PAY_CHANNEL))
	{
		strFileName = p_bank_attr->strBankType + "_ALIPAYZF" + getSysDate(starttime) + ".csv";
		strCipherFileName = p_bank_attr->strBankType + "_ALIPAYZF" + getSysDate(starttime) + ".SRC";
	}
	pSettleFile = new CBillFile(strsrcPath.c_str(), strFileName.c_str());
	pSettleCipherFile = new CBillFile(strEncryptPath.c_str(), strCipherFileName.c_str());
	std::string sSettlePathFileName = strsrcPath + strFileName;
	std::string sSettleCipherPathFileName = strEncryptPath + strCipherFileName;
	CDEBUG_LOG("sSettlePathFileName:[%s] sSettleCipherPathFileName[%s] \n",
			sSettlePathFileName.c_str(), sSettleCipherPathFileName.c_str());

	//../client/speedpos_bill/data/paybill/1001/src/
	//加密文件会被移走，检查未加密的文件
	if (access(sSettlePathFileName.c_str(), F_OK) == 0)
	{
		CERROR_LOG("CProcBillContrastTask::HandleProcess:settle file has exists! "
			"Ret[%d].\n",
			iRet);
		throw(CTrsExp(ERR_BILL_FILE_EXIST,errMap[ERR_BILL_FILE_EXIST]));
	}
	//bm 下的目录文件校验
	std::string sShopBillSrcFillPath = mainConfig.sSftpBillPath + m_Req.sBmId + "/" + SRC_PATH +
			mainConfig.sSftMchBillFilePrefix;
	std::string sChannelBillSrcFillPath = mainConfig.sSftpBillPath + m_Req.sBmId + "/" + SRC_PATH +
			mainConfig.sSftChannelBillFilePrefix;
	std::string sWxBillSrcFillPath = mainConfig.sSftpBillPath + m_Req.sBmId + "/" + SRC_PATH +
			mainConfig.sSftWXBillFilePrefix;
	std::string sAliBillDecFilePath = mainConfig.sSftpBillPath + m_Req.sBmId + "/" + SRC_PATH +
			mainConfig.sSftAliBillFilePrefix;

	pBillShopFille 		= new CBillFile(sShopBillSrcFillPath.c_str(), starttime);
	pBillChannelFille 	= new CBillFile(sChannelBillSrcFillPath.c_str(), starttime);
	pBillWxFile 		= new CBillFile(sWxBillSrcFillPath.c_str(), starttime);
	pBillAliFile 		= new CBillFile(sAliBillDecFilePath.c_str(), starttime);

	sShopBillSrcFileName 	= sShopBillSrcFillPath + "_" + getSysDate(starttime) + ".csv";
	sChannelBillSrcFileName = sChannelBillSrcFillPath + "_" + getSysDate(starttime) + ".csv";
	sWxBillSrcFileName 		= sWxBillSrcFillPath + "_" + getSysDate(starttime) + ".csv";
	sAliBillSrcFileName 	= sAliBillDecFilePath + "_" + getSysDate(starttime) + ".csv";

	CDEBUG_LOG("CheckBillFill ! "
		"sShopBillSrcFileName:[%s] sChannelBillSrcFileName:[%s] sWxBillSrcFileName[%s] sAliBillSrcFileName[%s].\n",
		sShopBillSrcFileName.c_str(), sChannelBillSrcFileName.c_str(), sWxBillSrcFileName.c_str(), sAliBillSrcFileName.c_str());

	//判断目录是否存在
	string temppath = mainConfig.sSftpBillPath + m_Req.sBmId + "/" + SRC_PATH;
	if(access(temppath.c_str(),F_OK) != 0)
	{
		//循环创建目录
		if(!tars::TC_File::makeDirRecursive(temppath))
		{
			CERROR_LOG("create dir [%s] failed!", temppath.c_str());
			throw(CTrsExp(ERR_FILE_DIR_CREATE_FAILED,errMap[ERR_FILE_DIR_CREATE_FAILED]));
		}
	}


}

void CBillContrastBase::Copy2GetFile(ProPullBillReq& m_Req,int starttime)
{
	BEGIN_LOG(__func__);
	CDEBUG_LOG("Copy2GetFile begin");

	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;

	std::string strSftpMchLongRangPath = mainConfig.sSftpLongRangPath + m_Req.sBmId + "/" + SRC_PATH + mainConfig.sSftMchBillFilePrefix
		+ "_" + getSysDate(starttime) + ".csv";
	std::string strSftpChannelLongRangPath = mainConfig.sSftpLongRangPath + m_Req.sBmId + "/" + SRC_PATH + mainConfig.sSftChannelBillFilePrefix
		+ "_" + getSysDate(starttime) + ".csv";
	std::string strSftpWxLongRangPath = mainConfig.sSftpLongRangPath + m_Req.sBmId + "/" + SRC_PATH +  mainConfig.sSftWXBillFilePrefix
		+ "_" + getSysDate(starttime) + ".csv";
	std::string strSftpAliLongRangPath = mainConfig.sSftpLongRangPath + m_Req.sBmId + "/" + SRC_PATH + mainConfig.sSftAliBillFilePrefix
		+ "_" + getSysDate(starttime) + ".csv";

	CDEBUG_LOG("use copy to get file..\n");
	CDEBUG_LOG("strSftpMchLongRangPath=[%s],sShopBillSrcFileName=[%s]",strSftpMchLongRangPath.c_str(),sShopBillSrcFileName.c_str());
	tars::TC_File::copyFile(strSftpMchLongRangPath,sShopBillSrcFileName,true);
	tars::TC_File::copyFile(strSftpChannelLongRangPath,sChannelBillSrcFileName,true);

	if(m_Req.sPayChannel == WX_API_PAY_CHANNEL)
	{
		CDEBUG_LOG("strSftpWxLongRangPath = [%s]",strSftpWxLongRangPath.c_str());
		tars::TC_File::copyFile(strSftpWxLongRangPath,sWxBillSrcFileName,true);
	}

	if(m_Req.sPayChannel == ALI_API_PAY_CHANNEL)
	{
		CDEBUG_LOG("strSftpAliLongRangPath = [%s]",strSftpAliLongRangPath.c_str());
		tars::TC_File::copyFile(strSftpAliLongRangPath,sAliBillSrcFileName,true);
		//转码    --hawrk 在下载的时候 已转
//		char szToBuf[256] = { 0 };
//		snprintf(szToBuf, sizeof(szToBuf), "iconv -f GBK -t UTF-8 %s -o %s\n", sAliBillSrcFileName.c_str(),sAliBillSrcFileName.c_str());
//		CDEBUG_LOG("szToBuf [%s]", szToBuf);
//		system(szToBuf);

	}

	TrimBillFile(m_Req);
}

void CBillContrastBase::SFTPDownLoad(ProPullBillReq& m_Req,int starttime)
{
	BEGIN_LOG(__func__);
	CDEBUG_LOG("SFTPDownLoad begin");
	INT32 iRet = 0;

	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;

	std::string strSftpMchLongRangPath = mainConfig.sSftpLongRangPath + m_Req.sBmId + "/" + SRC_PATH + mainConfig.sSftMchBillFilePrefix
		+ "_" + getSysDate(starttime) + ".csv";
	std::string strSftpChannelLongRangPath = mainConfig.sSftpLongRangPath + m_Req.sBmId + "/" + SRC_PATH + mainConfig.sSftChannelBillFilePrefix
		+ "_" + getSysDate(starttime) + ".csv";
	std::string strSftpWxLongRangPath = mainConfig.sSftpLongRangPath + m_Req.sBmId + "/" + SRC_PATH +  mainConfig.sSftWXBillFilePrefix
		+ "_" + getSysDate(starttime) + ".csv";
	std::string strSftpAliLongRangPath = mainConfig.sSftpLongRangPath + m_Req.sBmId + "/" + SRC_PATH + mainConfig.sSftAliBillFilePrefix
		+ "_" + getSysDate(starttime) + ".csv";

	CDEBUG_LOG("use sftp to get file..\n");
	//先删除原来的文件
	if(access(sShopBillSrcFileName.c_str(),F_OK) == 0)
	{
		remove(sShopBillSrcFileName.c_str());
	}
	if(access(sChannelBillSrcFileName.c_str(),F_OK) == 0)
	{
		remove(sChannelBillSrcFileName.c_str());
	}
	if(access(sWxBillSrcFileName.c_str(),F_OK) == 0)
	{
		remove(sWxBillSrcFileName.c_str());
	}
	if(access(sAliBillSrcFileName.c_str(),F_OK) == 0)
	{
		remove(sAliBillSrcFileName.c_str());
	}
	//delete end
	//pBillShopFille->_open();

	//文件为追加！！！
	iRet = g_cOrderServer.ProcSftpDownLoad(mainConfig.sFtpUser.c_str(),
		mainConfig.sFtpPass.c_str(),
		mainConfig.sFtpIp.c_str(),
		mainConfig.iSftpPort,
		strSftpMchLongRangPath.c_str(),
		*pBillShopFille);
	if (iRet < 0)
	{
		CERROR_LOG("Shop bill file ProcSftpDownLoad failed !iRet =[%d]", iRet);
		throw(CTrsExp(ERR_DOWNLOAD_FILE,errMap[ERR_DOWNLOAD_FILE]));

	}

	iRet = g_cOrderServer.ProcSftpDownLoad(mainConfig.sFtpUser.c_str(),
		mainConfig.sFtpPass.c_str(),
		mainConfig.sFtpIp.c_str(),
		mainConfig.iSftpPort,
		strSftpChannelLongRangPath.c_str(),
		*pBillChannelFille);
	if (iRet < 0)
	{
		CERROR_LOG("channel bill file ProcSftpDownLoad failed !iRet =[%d]", iRet);
		throw(CTrsExp(ERR_DOWNLOAD_FILE,errMap[ERR_DOWNLOAD_FILE]));
	}

	if (0 == strcmp(m_Req.sPayChannel.c_str(), WX_API_PAY_CHANNEL))
	{
		//wx
		iRet = g_cOrderServer.ProcSftpDownLoad(mainConfig.sFtpUser.c_str(),
			mainConfig.sFtpPass.c_str(),
			mainConfig.sFtpIp.c_str(),
			mainConfig.iSftpPort,
			strSftpWxLongRangPath.c_str(),
			*pBillWxFile);
		if (iRet < 0)
		{
			CERROR_LOG("wx bill file ProcSftpDownLoad failed !iRet =[%d]", iRet);
			throw(CTrsExp(ERR_DOWNLOAD_FILE,errMap[ERR_DOWNLOAD_FILE]));
		}
	}

	if (0 == strcmp(m_Req.sPayChannel.c_str(), ALI_API_PAY_CHANNEL))
	{
		//ali
		iRet = g_cOrderServer.ProcSftpDownLoad(mainConfig.sFtpUser.c_str(),
			mainConfig.sFtpPass.c_str(),
			mainConfig.sFtpIp.c_str(),
			mainConfig.iSftpPort,
			strSftpAliLongRangPath.c_str(),
			*pBillAliFile);
		if (iRet < 0)
		{
			CERROR_LOG("ali bill file ProcSftpDownLoad failed !iRet =[%d]", iRet);
			throw(CTrsExp(ERR_DOWNLOAD_FILE,errMap[ERR_DOWNLOAD_FILE]));
		}
		//ali bill convert utf-8  --hawrk 下载的时候已转
//			CDEBUG_LOG("fileName %s \n", pBillAliFile->GetFileName().c_str());
//
//			void *memory = NULL;
//			int file_length = 0;
//			vector<std::string> vecBill;
//			int fd = open(pBillAliFile->GetFileName().c_str(), O_RDONLY);
//			if (fd < 0)
//			{
//				CERROR_LOG("file open [%s] error\n", pBillAliFile->GetFileName().c_str());
//				return -40;
//			}
//			file_length = lseek(fd, 1, SEEK_END);
//			memory = mmap(NULL, file_length, PROT_READ, MAP_SHARED, fd, 0);
//			split_ex((char *)memory, '\n', vecBill);
//			remove(pBillAliFile->GetFileName().c_str());
//			std::string sAliToUtfPath = mainConfig.sSftpBillPath + m_stReq.sBmId + "/" + SRC_PATH + mainConfig.sSftAliBillFilePrefix;
//			CBillFile* pAliToUtf = new CBillFile(sAliToUtfPath.c_str(), m_iBillBeginTime);
//			for (size_t iIndex = 0; iIndex < vecBill.size(); ++iIndex)
//			{
//				//CDEBUG_LOG("buf %s \n", vecBill[iIndex].c_str());
//				char szToUtf[1024] = { 0 };
//				code_convert("GB2312", "UTF-8", vecBill[iIndex].c_str(), strlen(vecBill[iIndex].c_str()), szToUtf, sizeof(szToUtf));
//				//CDEBUG_LOG("Utf-8 buf %s \n", szToUtf);
//				pAliToUtf->raw("%s\n", szToUtf);
//			}
	}

	TrimBillFile(m_Req);
}

void CBillContrastBase::TrimBillFile(ProPullBillReq& m_stReq)
{
	//清除原始对账单的一些垃圾字段
	CDEBUG_LOG("trim bill file begin");
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;

	std::string sPath = mainConfig.sSftpShellPath;
	CDEBUG_LOG("sPath : [%s] \n", sPath.c_str());

	//system bill
	char szLoadOrderFlowCmd[512] = { 0 };
	snprintf(szLoadOrderFlowCmd, sizeof(szLoadOrderFlowCmd), "sh %s%s %s shop",
		sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(),sShopBillSrcFileName.c_str());
	CDEBUG_LOG("szLoadOrderFlowCmd = [%s]",szLoadOrderFlowCmd);
	system(szLoadOrderFlowCmd);

	//channel bill
	char szLoadOrderChannelFlowCmd[512] = { 0 };
	snprintf(szLoadOrderChannelFlowCmd, sizeof(szLoadOrderChannelFlowCmd), "sh %s%s %s channel",
		sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(),  sChannelBillSrcFileName.c_str());
	CDEBUG_LOG("LoadOrderChannelFlowCmd:[%s] \n", szLoadOrderChannelFlowCmd);
	system(szLoadOrderChannelFlowCmd);

	//wx
	if (0 == strcmp(m_stReq.sPayChannel.c_str(), WX_API_PAY_CHANNEL))
	{
		char szLoadWxFlowCmd[512] = { 0 };
		snprintf(szLoadWxFlowCmd, sizeof(szLoadWxFlowCmd), "sh %s%s %s wx %s",
			sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(), sWxBillSrcFileName.c_str(), m_stReq.sBmId.c_str());
		CDEBUG_LOG("LoadWxFlowCmd:[%s] \n", szLoadWxFlowCmd);
		system(szLoadWxFlowCmd);
	}

	//ali
	if (0 == strcmp(m_stReq.sPayChannel.c_str(), ALI_API_PAY_CHANNEL))
	{
		char szLoadAliFlowCmd[512] = { 0 };
		snprintf(szLoadAliFlowCmd, sizeof(szLoadAliFlowCmd), "sh %s%s %s ali %s",
			sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(),sAliBillSrcFileName.c_str(), m_stReq.sBmId.c_str());
		CDEBUG_LOG("szLoadAliFlowCmd:[%s] \n", szLoadAliFlowCmd);
		system(szLoadAliFlowCmd);
	}

}

void CBillContrastBase::ProRemitBillProcess(ProPullBillReq& m_Req,int starttime)
{
	BEGIN_LOG(__func__);
	CDEBUG_LOG("ProRemitBillProcess begin");
	INT32 iRet = 0;
	int iIndex = 1;
	int total_bm_profit = 0;
	int total_service_profit = 0;
	int total_amount = 0;
	/** 计算开始时间  结束时间**/
	std::string strPayTime = getSysDate(starttime);

	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;


	CDEBUG_LOG("order trade bill num:[%d],refund bill num:[%d]",orderPayBillMap.size(),orderRefundBillMap.size());
	std::map<std::string, OrderRefundBillSumary>::iterator itRefund;
	for (std::map<std::string, OrderPayBillSumary>::iterator iter = orderPayBillMap.begin();
		iter != orderPayBillMap.end(); ++iter)
	//for(auto iter : orderPayBillMap)
	{
		OrderStat orderStat;
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
			orderStat.payment_net_profit -= itRefund->second.payment_profit;
			orderStat.channel_net_profit -= itRefund->second.channel_profit;
			orderStat.service_net_profit -= itRefund->second.service_profit;
			orderStat.bm_net_profit -= itRefund->second.bm_profit;
		}
		total_bm_profit += orderStat.bm_net_profit;
		total_service_profit += orderStat.service_net_profit;
		CDEBUG_LOG("OrderStat: mch_id[%s] trade_count[%d] trade_amount[%d] refund_count[%d] refund_amount[%d] trade_net_amount[%d]"
			" total_net_commission[%d] shop_net_amount[%d] payment_net_profit[%d] channel_net_profit[%d] service_net_profit[%d] bm_net_profit[%d] \n",
			orderStat.mch_id.c_str(), orderStat.trade_count, orderStat.trade_amount, orderStat.refund_count, orderStat.refund_amount,
			orderStat.trade_net_amount, orderStat.total_net_commission, orderStat.shop_net_amount, orderStat.payment_net_profit,
			orderStat.channel_net_profit, orderStat.service_net_profit, orderStat.bm_net_profit);
		if (0 < orderStat.shop_net_amount)
		{
			TRemitBill remitBill;
			remitBill.Reset();
			remitBill.account_id = orderStat.mch_id;
			remitBill.sType = SHOP_TYPE_NAME;
			remitBill.remit_fee = orderStat.shop_net_amount;
			remitBill.sRemitfee = F2Y(toString(orderStat.shop_net_amount));
			remitBill.sPayTime = toDate(strPayTime);
			remitBill.sRemitTime = toDate(getSysDate());
			remitBill.sRemark = SHOP_REMARK_NAME;

			AddSettleLog(m_Req,remitBill,tremitMap,iIndex,total_amount);
		}
	}
	//TODO: 考虑到大数据量时，orderPayBillMap&orderRefundBillMap会占用大量内存，后面没用到可以直接释放掉


	CDEBUG_LOG("bill channel trade num:[%d],refund num:[%d]",channelPayMap.size(),channelRefundMap.size());
	std::map<std::string, OrderChannelFlowData>::iterator it_refund;
	for (std::map<std::string, OrderChannelFlowData>::iterator iterCl = channelPayMap.begin(); iterCl != channelPayMap.end(); ++iterCl)
	{
		std::string channel_id = iterCl->first;
		int channel_profit = iterCl->second.channel_profit;
		CDEBUG_LOG("pay channel_id:[%s] channel_profit[%d]\n", channel_id.c_str(), channel_profit);
		if ((it_refund = channelRefundMap.find(channel_id)) != channelRefundMap.end())
		{
			CDEBUG_LOG("refund channel_id:[%s] channel_profit[%d]\n", channel_id.c_str(), it_refund->second.channel_profit);
			channel_profit -= it_refund->second.channel_profit;
		}
		if (channel_profit > 0)
		{
			TRemitBill remitBill;
			remitBill.Reset();
			remitBill.account_id = channel_id;
			remitBill.sType = CHANNEL_TYPE_NAME;
			remitBill.sRemitfee = F2Y(toString(channel_profit));
			remitBill.remit_fee = channel_profit;
			remitBill.sPayTime = toDate(strPayTime);
			remitBill.sRemitTime = toDate(getSysDate());
			remitBill.sRemark = CHANNEL_REMARK_NAME;

			AddSettleLog(m_Req,remitBill,tremitMap,iIndex,total_amount);

		}
	}
	//TODO:channelPayMap&channelRefundMap占用的内存考虑释放
	/**--------end---------------*/

	CDEBUG_LOG("total_service_profit:[%d] total_bm_profit[%d]\n", total_service_profit, total_bm_profit);
	if (0 < total_service_profit)
	{
		TRemitBill remitBill;
		remitBill.Reset();
		remitBill.account_id = m_Req.sBmId;
		remitBill.sType = SERVICE_TYPE_NAME;
		remitBill.sRemitfee = F2Y(toString(total_service_profit));
		remitBill.remit_fee = total_service_profit;
		remitBill.sPayTime = toDate(strPayTime);
		remitBill.sRemitTime = toDate(getSysDate());
		remitBill.sRemark = SERVICE_REMARK_NAME;

		AddSettleLog(m_Req,remitBill,tremitMap,iIndex,total_amount);

	}
	if (0 < total_bm_profit)
	{
		TRemitBill remitBill;
		remitBill.Reset();
		remitBill.account_id = m_Req.sBmId;
		remitBill.sType = BM_TYPE_NAME;
		remitBill.sRemitfee = F2Y(toString(total_bm_profit));
		remitBill.remit_fee = total_bm_profit;
		remitBill.sPayTime = toDate(strPayTime);
		remitBill.sRemitTime = toDate(getSysDate());
		remitBill.sRemark = BM_REMARK_NAME;

		AddSettleLog(m_Req,remitBill,tremitMap,iIndex,total_amount);

	}
	m_total_count = tremitMap.size();
	m_total_amount = total_amount;
	//获取以前未打款汇款单记录
	iRet = g_cOrderServer.CallGetPayFailApi(m_Req.sBmId, iIndex, m_Req.sPayChannel, strPayTime, tremitMap);
	if (iRet < 0)
	{
		CERROR_LOG("CallGetPayFailApi failed !iRet =[%d]", iRet);
		throw(CTrsExp(ERR_GET_FAILBILL_ERR,errMap[ERR_GET_FAILBILL_ERR]));
	}

	CDEBUG_LOG("creating statements file,currday bill num:[%d],total bill num:[%d]",m_total_count,tremitMap.size());
	//生成入账单
	for (std::map<int, TRemitBill>::iterator itRemit = tremitMap.begin(); itRemit != tremitMap.end(); ++itRemit)
	{
		//
		char szBuf[1024] = { 0 };
		snprintf(szBuf, sizeof(szBuf), "%d,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
			itRemit->first, itRemit->second.sPayTime.c_str(), itRemit->second.account_id.c_str(), itRemit->second.sType.c_str(),itRemit->second.sName.c_str(),
			itRemit->second.sBankOwner.c_str(), itRemit->second.sBankCardNo.c_str(), itRemit->second.sBankCardType.c_str(),
			itRemit->second.sBankType.c_str(),itRemit->second.sBranchNo.c_str(),itRemit->second.sRemitfee.c_str(),
			itRemit->second.sRemitTime.c_str(),itRemit->second.sRemark.c_str());
		pSettleFile->raw(szBuf);
	}

}


void CBillContrastBase::AddSettleLog(ProPullBillReq& m_Req,TRemitBill& remitBill,std::map<int, TRemitBill>& tremitMap,int& iIndex,int& total_amount)
{
	CDEBUG_LOG("AddSettleLog begin");
	int iRet;
	iRet = g_cOrderServer.CallGetBankNoApi(remitBill);
	if (iRet < 0)
	{
		CERROR_LOG("CallGetBankNoApi failed! iRet[%d].\n",iRet);
		throw(CTrsExp(ERR_NOTIFY_SETTLE_FAILED,errMap[ERR_NOTIFY_SETTLE_FAILED]));
	}
	iRet = g_cOrderServer.CallAddSettleLogApi(m_Req.sBmId, m_Req.sPayChannel, remitBill);
	if (iRet >= 0)
	{
		total_amount += remitBill.remit_fee;
		tremitMap.insert(std::make_pair(iIndex, remitBill));
		++iIndex;
	}
}

