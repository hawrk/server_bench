/*
 * CBillConstrastCommon.cpp
 *
 *  Created on: 2017年6月2日
 *      Author: hawrkchen
 */

#include "CBillConstrastCommon.h"

extern CSpeedPosServer g_cOrderServer;

CBillContrastCommon::CBillContrastCommon()
{
	//TODO:
	CDEBUG_LOG("CBillContrastCommon begin");
	checkedbillFile = NULL;
	pBillDb = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
}

CBillContrastCommon::~CBillContrastCommon()
{
	//TODO:
	CDEBUG_LOG("~CBillContrastCommon begin");

	if(NULL != checkedbillFile)
	{
		delete checkedbillFile;
		checkedbillFile = NULL;
	}
	//pBillDb 单例模式下自行管理，不需要手动delete
//	if(NULL != pBillDb)
//	{
//		delete pBillDb;
//		pBillDb = NULL;
//	}
}

void CBillContrastCommon::BillFileDownLoad(ProPullBillReq& m_stReq,int starttime)
{
	//
	int iRet;
	//同步对账步骤
	iRet = g_cOrderServer.CallUpdateBillContrastApi(m_stReq.sBmId, m_stReq.sPayChannel, toDate(getSysDate(starttime)), BILL_CONTRAST_STEP_BEGIN_SFTP_DOWNLOAD,
													stepMap[BILL_CONTRAST_STEP_BEGIN_SFTP_DOWNLOAD]);
	if (iRet < 0)
	{
		CERROR_LOG("CallAddBillContrastApi failed!iRet =[%d]", iRet);
		throw(CTrsExp(ERR_ORDER_NO_MAPPING_EXIST,errMap[ERR_ORDER_NO_MAPPING_EXIST]));
	}


	Copy2GetFile(m_stReq,starttime);
	//SFTPDownLoad(m_stReq,starttime);
}

void CBillContrastCommon::Copy2GetFile(ProPullBillReq& m_Req,int starttime)
{
	BEGIN_LOG(__func__);
	CDEBUG_LOG("Copy2GetFile begin");
	int iRet;

	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;
	//获取多个文件
	clib_mysql* pBillDb = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
	//CBillBusiConfig* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
	CMySQL m_mysql;
	ostringstream sqlss;
	SqlResultMapVector outmapVector;

    sqlss.str("");
    sqlss <<"select gateway_id  "
    	  <<" from "<<SHOP_DB<<"."<<SHOP_GATEWAY
		  <<" where bm_id ='"<<m_Req.sBmId<<"' and pay_channel ='"<<m_Req.sPayChannel<<"' and status = '1';";

    iRet = m_mysql.QryAndFetchResMVector(*pBillDb,sqlss.str().c_str(),outmapVector);
    if(iRet == 1)
    {
    	for(size_t i = 0;i < outmapVector.size(); i++)
    	{
    		if(m_Req.sPayChannel == WX_API_PAY_CHANNEL)
    		{
        		std::string strSftpWxLongRangPath = mainConfig.sSftpLongRangPath + m_Req.sBmId + "/" + SRC_PATH +  mainConfig.sWXBillSrcPrefix
        			+ "_" + outmapVector[i]["gateway_id"] + "_" + getSysDate(starttime) + ".csv";
        		CDEBUG_LOG("strSftpWxLongRangPath = [%s]",strSftpWxLongRangPath.c_str());

//        		if(tars::TC_File::getFileSize(strSftpWxLongRangPath) == 0)
//        		{
//        			//无数据，则跳过这个文件
//        			CDEBUG_LOG("bill file name  [%s] no data ,jump over!!",strSftpWxLongRangPath.c_str());
//        			continue;
//        		}

        		if(!tars::TC_File::isFileExist(strSftpWxLongRangPath))  //对账文件不存在，则跳过
        		{
        			CERROR_LOG("source file :[%s] not exist!!", strSftpWxLongRangPath.c_str());
        			continue;
        			//throw(CTrsExp(ERR_DETAIL_FILE_NOT_FOUND,errMap[ERR_DETAIL_FILE_NOT_FOUND]));
        		}

        		std::string strWxBillSrcFileName = mainConfig.sSftpBillPath + m_Req.sBmId + "/" + SRC_PATH + mainConfig.sWXBillSrcPrefix
        				+ "_" + outmapVector[i]["gateway_id"] + "_" + getSysDate(starttime) + ".csv";

        		tars::TC_File::copyFile(strSftpWxLongRangPath,strWxBillSrcFileName,true);
        		m_file_vec.push_back(strWxBillSrcFileName);
    		}
    		if(m_Req.sPayChannel == ALI_API_PAY_CHANNEL)
    		{
    			std::string strSftpAliLongRangPath = mainConfig.sSftpLongRangPath + m_Req.sBmId + "/" + SRC_PATH + mainConfig.sAliBillSrcPrefix
    				+ "_" + outmapVector[i]["gateway_id"] + "_" + getSysDate(starttime) + ".csv";
    			CDEBUG_LOG("strSftpAliLongRangPath = [%s]",strSftpAliLongRangPath.c_str());

        		if(!tars::TC_File::isFileExist(strSftpAliLongRangPath))
        		{
        			CERROR_LOG("source file :[%s] not exist!!", strSftpAliLongRangPath.c_str());
        			continue;
        			//throw(CTrsExp(ERR_DETAIL_FILE_NOT_FOUND,errMap[ERR_DETAIL_FILE_NOT_FOUND]));
        		}

        		std::string strAliBillSrcFileName = mainConfig.sSftpBillPath + m_Req.sBmId + "/" + SRC_PATH + mainConfig.sAliBillSrcPrefix
        				+ "_" + outmapVector[i]["gateway_id"] + "_" + getSysDate(starttime) + ".csv";

    			tars::TC_File::copyFile(strSftpAliLongRangPath,strAliBillSrcFileName,true);

    			m_file_vec.push_back(strAliBillSrcFileName);
    		}

    	}
    }

	std::string strSftpMchLongRangPath = mainConfig.sSftpLongRangPath + m_Req.sBmId + "/" + SRC_PATH + mainConfig.sSftMchBillFilePrefix
		+ "_" + getSysDate(starttime) + ".csv";
	std::string strSftpChannelLongRangPath = mainConfig.sSftpLongRangPath + m_Req.sBmId + "/" + SRC_PATH + mainConfig.sSftChannelBillFilePrefix
		+ "_" + getSysDate(starttime) + ".csv";
//	std::string strSftpWxLongRangPath = mainConfig.sSftpLongRangPath + m_Req.sBmId + "/" + SRC_PATH +  mainConfig.sSftWXBillFilePrefix
//		+ "_" + getSysDate(starttime) + ".csv";
//	std::string strSftpAliLongRangPath = mainConfig.sSftpLongRangPath + m_Req.sBmId + "/" + SRC_PATH + mainConfig.sSftAliBillFilePrefix
//		+ "_" + getSysDate(starttime) + ".csv";

	CDEBUG_LOG("strSftpMchLongRangPath=[%s],sShopBillSrcFileName=[%s]",strSftpMchLongRangPath.c_str(),sShopBillSrcFileName.c_str());
	if(!tars::TC_File::isFileExist(strSftpMchLongRangPath)||!tars::TC_File::isFileExist(strSftpChannelLongRangPath))
	{
		CERROR_LOG("source file :[%s]/[%s] not exist!!", strSftpMchLongRangPath.c_str(),strSftpChannelLongRangPath.c_str());
		throw(CTrsExp(ERR_DETAIL_FILE_NOT_FOUND,errMap[ERR_DETAIL_FILE_NOT_FOUND]));
	}
	tars::TC_File::copyFile(strSftpMchLongRangPath,sShopBillSrcFileName,true);
	tars::TC_File::copyFile(strSftpChannelLongRangPath,sChannelBillSrcFileName,true);

//	if(m_Req.sPayChannel == WX_API_PAY_CHANNEL)
//	{
//		CDEBUG_LOG("strSftpWxLongRangPath = [%s]",strSftpWxLongRangPath.c_str());
//		tars::TC_File::copyFile(strSftpWxLongRangPath,sWxBillSrcFileName,true);
//	}
//
//	if(m_Req.sPayChannel == ALI_API_PAY_CHANNEL)
//	{
//		CDEBUG_LOG("strSftpAliLongRangPath = [%s]",strSftpAliLongRangPath.c_str());
//		tars::TC_File::copyFile(strSftpAliLongRangPath,sAliBillSrcFileName,true);
//		//转码    --hawrk 在下载的时候 已转
////		char szToBuf[256] = { 0 };
////		snprintf(szToBuf, sizeof(szToBuf), "iconv -f GBK -t UTF-8 %s -o %s\n", sAliBillSrcFileName.c_str(),sAliBillSrcFileName.c_str());
////		CDEBUG_LOG("szToBuf [%s]", szToBuf);
////		system(szToBuf);
//
//	}

	TrimAllBillFile(m_Req);
}

void CBillContrastCommon::TrimAllBillFile(ProPullBillReq& m_stReq)
{
	//清除原始对账单的一些垃圾字段
	//不继承自基类的 TrimBillFile
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

	//微信&支付宝
	string channel;
	if (0 == strcmp(m_stReq.sPayChannel.c_str(), WX_API_PAY_CHANNEL))
	{
		channel = "wx";
	}
	else
	{
		channel = "ali";
	}
	for(vector<string>::iterator iter = m_file_vec.begin();iter != m_file_vec.end();iter++)
	{
		char szLoadWxFlowCmd[512] = { 0 };
		snprintf(szLoadWxFlowCmd, sizeof(szLoadWxFlowCmd), "sh %s%s %s %s %s",
			sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(), (*iter).c_str(), channel.c_str(),m_stReq.sBmId.c_str());
		CDEBUG_LOG("LoadWxFlowCmd:[%s] \n", szLoadWxFlowCmd);
		system(szLoadWxFlowCmd);
	}


	//wx
//	if (0 == strcmp(m_stReq.sPayChannel.c_str(), WX_API_PAY_CHANNEL))
//	{
//		char szLoadWxFlowCmd[512] = { 0 };
//		snprintf(szLoadWxFlowCmd, sizeof(szLoadWxFlowCmd), "sh %s%s %s wx %s",
//			sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(), sWxBillSrcFileName.c_str(), m_stReq.sBmId.c_str());
//		CDEBUG_LOG("LoadWxFlowCmd:[%s] \n", szLoadWxFlowCmd);
//		system(szLoadWxFlowCmd);
//
//
//	}
//
//	//ali
//	if (0 == strcmp(m_stReq.sPayChannel.c_str(), ALI_API_PAY_CHANNEL))
//	{
//		char szLoadAliFlowCmd[512] = { 0 };
//		snprintf(szLoadAliFlowCmd, sizeof(szLoadAliFlowCmd), "sh %s%s %s ali %s",
//			sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(),sAliBillSrcFileName.c_str(), m_stReq.sBmId.c_str());
//		CDEBUG_LOG("szLoadAliFlowCmd:[%s] \n", szLoadAliFlowCmd);
//		system(szLoadAliFlowCmd);
//	}
}

void CBillContrastCommon::LoadBillFlowToDB(ProPullBillReq& m_stReq,int starttime)
{
	BEGIN_LOG(__func__);
	CDEBUG_LOG("LoadBillFlowToDB begin");
	INT32 iRet = 0;

	//校验对账文件是否已下载
	if(access(sShopBillSrcFileName.c_str(),F_OK) != 0
			||access(sChannelBillSrcFileName.c_str(),F_OK) != 0)
	{
		CERROR_LOG("download file not exist!!");
		throw(CTrsExp(ERR_DETAIL_FILE_NOT_FOUND,errMap[ERR_DETAIL_FILE_NOT_FOUND]));
	}

	if(m_file_vec.empty())
	{
		CERROR_LOG("wx/ali bill file not exist!!");
		throw(CTrsExp(ERR_DETAIL_FILE_NOT_FOUND,"wx/ali bill file not exist!!"));
	}

//	if(m_stReq.sPayChannel == WX_API_PAY_CHANNEL)
//	{
//		if(access(sWxBillSrcFileName.c_str(),F_OK) != 0)
//		{
//			CERROR_LOG("wx bill file not exist!!");
//			throw(CTrsExp(ERR_DETAIL_FILE_NOT_FOUND,"wx bill file not exist!!"));
//		}
//	}
//	if(m_stReq.sPayChannel == ALI_API_PAY_CHANNEL)
//	{
//		if(access(sAliBillSrcFileName.c_str(),F_OK) != 0)
//		{
//			CERROR_LOG("ali bill file not exist!!");
//			throw(CTrsExp(ERR_DETAIL_FILE_NOT_FOUND,"ali bill file not exist!!"));
//		}
//	}

	//同步对账步骤
	//TODO
	iRet = g_cOrderServer.CallUpdateBillContrastApi(m_stReq.sBmId, m_stReq.sPayChannel, toDate(getSysDate(starttime)), BILL_CONTRAST_STEP_BEGIN_LOAD_BILL_FLOW,
					stepMap[BILL_CONTRAST_STEP_BEGIN_LOAD_BILL_FLOW]);
	if (iRet < 0)
	{
		CERROR_LOG("CallAddBillContrastApi failed!iRet =[%d]", iRet);
		throw(CTrsExp(ERR_ORDER_NO_MAPPING_EXIST,errMap[ERR_ORDER_NO_MAPPING_EXIST]));
	}
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

	//装载本地系统对账数据
	char szLoadOrderFlowCmd[512] = { 0 };
	snprintf(szLoadOrderFlowCmd, sizeof(szLoadOrderFlowCmd), "sh %s%s %s %d %s %s %s %s %s",
		sPath.c_str(), mainConfig.sBillTruncateDBShellPath.c_str(), pBillDb->ms_host, pBillDb->mi_port,
		pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(),sShopBillSrcFileName.c_str(),ORDER_ALL_FLOW);
	CDEBUG_LOG("szLoadOrderFlowCmd system: [%s]",szLoadOrderFlowCmd);
	system(szLoadOrderFlowCmd);

	//装载本地渠道对账数据
	memset(szLoadOrderFlowCmd,0x00,sizeof(szLoadOrderFlowCmd));
	snprintf(szLoadOrderFlowCmd, sizeof(szLoadOrderFlowCmd), "sh %s%s %s %d %s %s %s %s %s",
		sPath.c_str(), mainConfig.sBillTruncateDBShellPath.c_str(), pBillDb->ms_host, pBillDb->mi_port,
		pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(),sChannelBillSrcFileName.c_str(),ORDER_CHANNEL_FLOW);
	CDEBUG_LOG("szLoadOrderFlowCmd channel : [%s]",szLoadOrderFlowCmd);
	system(szLoadOrderFlowCmd);

	if (0 == strcmp(m_stReq.sPayChannel.c_str(), WX_API_PAY_CHANNEL))
	{
		for(vector<string>::iterator iter = m_file_vec.begin();iter != m_file_vec.end();iter++)
		{
			memset(szLoadOrderFlowCmd,0x00,sizeof(szLoadOrderFlowCmd));
			snprintf(szLoadOrderFlowCmd, sizeof(szLoadOrderFlowCmd), "sh %s%s %s %d %s %s %s %s %s",
				sPath.c_str(), mainConfig.sBillTruncateDBShellPath.c_str(), pBillDb->ms_host, pBillDb->mi_port,
				pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(),(*iter).c_str(),BILL_WXPAY_FLOW);
			CDEBUG_LOG("szLoadOrderFlowCmd wx : [%s]",szLoadOrderFlowCmd);
			system(szLoadOrderFlowCmd);
		}

	}

	if (0 == strcmp(m_stReq.sPayChannel.c_str(), ALI_API_PAY_CHANNEL))
	{
		for(vector<string>::iterator iter = m_file_vec.begin();iter != m_file_vec.end();iter++)
		{
			memset(szLoadOrderFlowCmd,0x00,sizeof(szLoadOrderFlowCmd));
			snprintf(szLoadOrderFlowCmd, sizeof(szLoadOrderFlowCmd), "sh %s%s %s %d %s %s %s %s %s",
				sPath.c_str(), mainConfig.sBillTruncateDBShellPath.c_str(), pBillDb->ms_host, pBillDb->mi_port,
				pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(),(*iter).c_str(),BILL_ALIPAY_FLOW);
			CDEBUG_LOG("szLoadOrderFlowCmd ali : [%s]",szLoadOrderFlowCmd);
			system(szLoadOrderFlowCmd);
		}
	}
	//char* path_end;
//	char dir[256] = { 0 };
//	//int n = readlink("/proc/self/exe", dir, 256);
//	readlink("/proc/self/exe",dir,256);
//	std::string sDir = dir;
//	int nLen = sDir.rfind('/');
//	std::string sPath = sDir.substr(0, nLen + 1);
//	std::string sPath = mainConfig.sSftpShellPath;
//	CDEBUG_LOG("sPath : [%s] \n", sPath.c_str());
//
//	char szLoadOrderFlowCmd[512] = { 0 };
//	snprintf(szLoadOrderFlowCmd, sizeof(szLoadOrderFlowCmd), "sh %s%s %s %d %s %s %s %s t_order_all_flow shop",
//		sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(), pBillDb->ms_host, pBillDb->mi_port,
//		pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(),sShopBillSrcFileName.c_str());
//	CDEBUG_LOG("szLoadOrderFlowCmd = [%s]",szLoadOrderFlowCmd);
//
//	char szLoadWxFlowCmd[512] = { 0 };
//	snprintf(szLoadWxFlowCmd, sizeof(szLoadWxFlowCmd), "sh %s%s %s %d %s %s %s %s t_wxpay_flow wx %s",
//		sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(), pBillDb->ms_host, pBillDb->mi_port,
//		pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(), sWxBillSrcFileName.c_str(), m_stReq.sBmId.c_str());
//	CDEBUG_LOG("LoadWxFlowCmd:[%s] \n", szLoadWxFlowCmd);
//
//	char szLoadOrderChannelFlowCmd[512] = { 0 };
//	snprintf(szLoadOrderChannelFlowCmd, sizeof(szLoadOrderChannelFlowCmd), "sh %s%s %s %d %s %s %s %s t_order_channel_all_flow channel",
//		sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(), pBillDb->ms_host, pBillDb->mi_port,
//		pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(), sChannelBillSrcFileName.c_str());
//	CDEBUG_LOG("LoadOrderChannelFlowCmd:[%s] \n", szLoadOrderChannelFlowCmd);
//
//	char szLoadAliFlowCmd[512] = { 0 };
//	snprintf(szLoadAliFlowCmd, sizeof(szLoadAliFlowCmd), "sh %s%s %s %d %s %s %s %s t_alipay_flow ali %s",
//		sPath.c_str(), mainConfig.sBillFileToDBShellPath.c_str(), pBillDb->ms_host, pBillDb->mi_port,
//		pBillDb->ms_user, pBillDb->ms_pass, strDbName.c_str(), sAliBillSrcFileName.c_str(),m_stReq.sBmId.c_str());
//	CDEBUG_LOG("szLoadAliFlowCmd:[%s] \n", szLoadAliFlowCmd);
//
//	system(szLoadOrderFlowCmd);
//	system(szLoadOrderChannelFlowCmd);
//	if (0 == strcmp(m_stReq.sPayChannel.c_str(), WX_API_PAY_CHANNEL))
//	{
//		system(szLoadWxFlowCmd);
//	}
//	if (0 == strcmp(m_stReq.sPayChannel.c_str(), ALI_API_PAY_CHANNEL))
//	{
//		system(szLoadAliFlowCmd);
//	}
}

void CBillContrastCommon::GetRemitBillData(ProPullBillReq& m_stReq,int starttime,int endtime)
{
	CDEBUG_LOG("GetRemitBillData begin");
	INT32 iRet = 0;
	std::string sBeginTime = getSysTime(starttime);
	std::string sEndTime = getSysTime(endtime);

	//先生成对账后的对账单
	CreateCheckedBillFile(m_stReq,starttime,endtime);


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
void CBillContrastCommon::CreateCheckedBillFile(ProPullBillReq& m_stReq,int starttime,int endtime)
{
	CDEBUG_LOG("CreateCheckedBillFile begin");
	int iRet = 0;
	char szLinBuf[4096] = { 0 };
	std::string sBeginTime = getSysTime(starttime);
	std::string sEndTime = getSysTime(endtime);
	//生成对账成功的对账单
	iRet = m_stTransFlowDao.GetCheckedBillData(*pBillDb,m_stReq.sBmId,m_stReq.sPayChannel,
							sBeginTime,sEndTime,checkbilldata);
	if(iRet < 0)
	{
		CERROR_LOG("GetCheckedBillData failed!iRet =[%d]", iRet);
		return ;  //暂时不要抛异常
		//throw(CTrsExp(ERR_QUERY_RECORD_ERR,errMap[ERR_QUERY_RECORD_ERR]));
	}


	//获取文件路径
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;
	//const CBillBusiConfig::BankAttr* p_bank_attr = pBillBusConfig->GetBankAttrCfg(m_stReq.sBmId);
	string strcheckedpath = mainConfig.sSftpBillPath + m_stReq.sBmId + "/checked/";
	CDEBUG_LOG("checked bill path = [%s]",strcheckedpath.c_str());
	if(access(strcheckedpath.c_str(),F_OK) != 0)
	{
		//循环创建目录
		if(!tars::TC_File::makeDirRecursive(strcheckedpath))
		{
			CERROR_LOG("create dir [%s] failed!", strcheckedpath.c_str());
			throw(CTrsExp(ERR_FILE_DIR_CREATE_FAILED,errMap[ERR_FILE_DIR_CREATE_FAILED]));
		}

	}
	string strchecked_file = getSysDate(starttime) + ".csv";
	checkedbillFile = new CBillFile(strcheckedpath.c_str(), strchecked_file.c_str());

	//判断 一下文件是否已存在，如果已存在，则不加文件
	if(!tars::TC_File::isFileExist(strcheckedpath+strchecked_file))
	{
		checkedbillFile->raw("%s\n", CHECKED_BILL_FILE_HEAD);//加文件头
	}

	for(vector<CheckedBillData>::iterator iter = checkbilldata.begin();iter != checkbilldata.end();iter++)
	{
		memset(szLinBuf, 0x0, sizeof(szLinBuf));
		snprintf(szLinBuf, sizeof(szLinBuf), "%s,%s,%s,%s,%s,"
				"%s,%s,%s,%s,%s,"
				"%s,%s,%s,%s,%s,"
				"%s,%s,%s,%s,%s,"
				"%s,%s,",
				iter->bm_id.c_str(),iter->pay_time.c_str(),iter->order_no.c_str(),iter->out_order_no.c_str(),iter->transaction_id.c_str(),
				iter->mch_id.c_str(),iter->channel_id.c_str(),iter->pay_channel.c_str(),iter->trade_type.c_str(),iter->order_status.c_str(),
				iter->total_fee.c_str(),iter->total_commission.c_str(),iter->shop_amount.c_str(),iter->refund_fee.c_str(),iter->refund_no.c_str(),
				iter->out_refund_no.c_str(),iter->refund_id.c_str(),iter->payment_profit.c_str(),iter->channel_profit.c_str(),iter->bm_profit.c_str(),
				iter->service_profit.c_str(),iter->sub_body.c_str());

		//生成支付交易对账单
		//CDEBUG_LOG("checked bill write szBuf[%s]!", szLinBuf);
		checkedbillFile->raw("%s\n", szLinBuf);//
	}

}

void CBillContrastCommon::ProcBillComparison(ProPullBillReq& m_stReq,int starttime,int endtime)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	CDEBUG_LOG("ProcBillComparison: Begin");
	//TODO
	iRet = g_cOrderServer.CallUpdateBillContrastApi(m_stReq.sBmId, m_stReq.sPayChannel, toDate(getSysDate(starttime)), BILL_CONTRAST_STEP_BEGIN_RECONCILIATION,
			   stepMap[BILL_CONTRAST_STEP_BEGIN_RECONCILIATION]);
	if (iRet < 0)
	{
		CERROR_LOG("CallAddBillContrastApi failed!iRet =[%d]", iRet);
		throw(CTrsExp(ERR_ORDER_NO_MAPPING_EXIST,errMap[ERR_ORDER_NO_MAPPING_EXIST]));
	}
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
				int problem_type = 2;  //渠道多
				StringMap paramMap;
				std::string strBody;
				CDEBUG_LOG("Wxpay OverFlow order_no:[%s]\n", wxOverFlowVec[iIndex].order_no.c_str());

				//paramMap.insert(StringMap::value_type("ver", "1"));
				paramMap.insert(StringMap::value_type("sync_flag", "0"));   //不同步更新
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

				tinyxml2::XMLDocument rsp_doc;
				if (tinyxml2::XML_SUCCESS != rsp_doc.Parse(strBody.c_str(), strBody.size()))
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
					StringMap orderMap;
					const char* ret_content = g_cOrderServer.GetXmlField(xmlElement, "content");
					cJSON* root = cJSON_Parse(ret_content);
					orderMap["bm_id"]  				= cJSON_GetObjectItem(root, "bm_id")->valuestring;
					orderMap["pay_channel"]  		= cJSON_GetObjectItem(root, "pay_channel")->valuestring;
					//校验银行编号和渠道
					if(orderMap["bm_id"] != m_stReq.sBmId || orderMap["pay_channel"] != m_stReq.sPayChannel)
					{
						CERROR_LOG("ret_msg bm_id or channel not match bm_id：[%s]--[%s],channel:[%s]---[%s]!",
								orderMap["bm_id"].c_str(),m_stReq.sBmId.c_str(),
								orderMap["pay_channel"].c_str(),m_stReq.sPayChannel.c_str());
						//银行编号或渠道号不一致 ，还是属于渠道多 problem_type = 2
					}
					else
					{
						problem_type = 3;  //状态不一致
					}

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

				int problem_type = 2;  //渠道多
				StringMap paramMap;
				std::string strBody;
				CDEBUG_LOG("Alipay OverFlow order_no:[%s]\n", iter->order_no.c_str());

				//paramMap.insert(StringMap::value_type("ver", "1"));
				paramMap.insert(StringMap::value_type("sync_flag", "0"));   //不同步更新
				paramMap.insert(StringMap::value_type("order_no", iter->order_no));
				paramMap.insert(StringMap::value_type("order_status", iter->order_status));
				if(iter->order_status == "REFUND")
				{
					//paramMap.insert(StringMap::value_type("refund_status", wxOverFlowVec[iIndex].refund_status));
					paramMap.insert(StringMap::value_type("refund_no", iter->refund_no));
				}

				iRet = g_cOrderServer.SendRequestTradeServapi(mainConfig.sSignKey,mainConfig.sTradeServQryOrderUrl,paramMap,strBody);
				if(strBody.empty())
				{
					CERROR_LOG("Query TradeService no response!");
					throw CTrsExp(ERR_PASE_RETRUN_DATA,"Query TradeService no response!");
				}

				tinyxml2::XMLDocument rsp_doc;
				if (tinyxml2::XML_SUCCESS != rsp_doc.Parse(strBody.c_str(), strBody.size()))
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
					StringMap orderMap;
					const char* ret_content = g_cOrderServer.GetXmlField(xmlElement, "content");
					cJSON* root = cJSON_Parse(ret_content);
					orderMap["bm_id"]  				= cJSON_GetObjectItem(root, "bm_id")->valuestring;
					orderMap["pay_channel"]  		= cJSON_GetObjectItem(root, "pay_channel")->valuestring;
					//校验银行编号和渠道
					if(orderMap["bm_id"] != m_stReq.sBmId || orderMap["pay_channel"] != m_stReq.sPayChannel)
					{
						CERROR_LOG("ret_msg bm_id or channel not match bm_id：[%s]--[%s],channel:[%s]---[%s]!",
								orderMap["bm_id"].c_str(),m_stReq.sBmId.c_str(),
								orderMap["pay_channel"].c_str(),m_stReq.sPayChannel.c_str());
						//银行编号或渠道号不一致 ，还是属于渠道多 problem_type = 2
					}
					else
					{
						problem_type = 3;  //状态不一致
					}
				}
				g_cOrderServer.CallAlipayExceptionOrderMsgApi(m_stReq.sBmId, problem_type, *iter);
			}
			throw(CTrsExp(ERR_ALIPAY_NOT_RECONCLIED,errMap[ERR_ALIPAY_NOT_RECONCLIED]));
		}
	}
}


void CBillContrastCommon::UpdateBillStatus(ProPullBillReq& m_stReq,int starttime)
{
	CDEBUG_LOG("UpdateBillStatus begin");
	int iRet;
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


		CDEBUG_LOG("encrypt file name:[%s],cipher file name=[%s]",
				pSettleFile->GetFileName().c_str(),pSettleCipherFile->GetFileName().c_str());
		DES_Encrypt(pSettleFile->GetFileName().c_str(),pSettleCipherFile->GetFileName().c_str());
//		else
//		{
//			tars::TC_File::copyFile(pSettleFile->GetFileName(),pSettleCipherFile->GetFileName(),true);
//			//CopyFile(pSettleFile->GetFileName().c_str(),pSettleCipherFile->GetFileName().c_str()); //覆盖
//		}

		//更新结算单状态
		for (std::map<int, TRemitBill>::iterator itRemit = tremitMap.begin(); itRemit != tremitMap.end(); ++itRemit)
		{
			iRet = g_cOrderServer.CallUpdateSettleLogApi(m_stReq.sBmId, m_stReq.sPayChannel, itRemit->second, 2);
			if (iRet < 0)
			{

				CERROR_LOG("CallUpdateSettleLogApi failed!iRet =[%d]", iRet);
				throw(CTrsExp(ERR_UPDATE_SETTLE_STATUS,errMap[ERR_UPDATE_SETTLE_STATUS]));
			}
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
			CERROR_LOG("CallUpdateBillContrastApi failed!iRet =[%d]", iRet);
			throw(CTrsExp(ERR_UPDATE_BILL_STATUS,errMap[ERR_UPDATE_BILL_STATUS]));
		}
	}
}

void CBillContrastCommon::InitBillFileDownLoad(ProPullBillReq& m_Req,int starttime)
{
	//初始化对账步骤
	int iRet;
	iRet = g_cOrderServer.CallAddBillContrastApi(m_Req.sBmId, m_Req.sPayChannel, toDate(getSysDate(starttime)),
				BILL_CONTRAST_STEP_BEGIN_ING,
				stepMap[BILL_CONTRAST_STEP_BEGIN_ING]);
	if (iRet < 0)
	{
		CERROR_LOG("CallAddBillContrastApi failed!iRet =[%d]", iRet);
		throw(CTrsExp(ERR_UPDATE_BILL_STATUS,errMap[ERR_UPDATE_BILL_STATUS]));

	}

}

void CBillContrastCommon::ProcException(ProPullBillReq& m_stReq,int starttime,int exce_type)
{
	g_cOrderServer.CallUpdateBillContrastApi(m_stReq.sBmId, m_stReq.sPayChannel, getSysDate(starttime),
		0, "", -1);
}


