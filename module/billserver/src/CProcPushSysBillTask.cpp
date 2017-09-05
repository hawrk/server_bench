/*
 * CIdGenTask.cpp
 *
 *  Created on: 2009-6-3
 *      Author: rogeryang
 */

#include <sys/time.h>
#include "CProcPushSysBillTask.h"
#include "tools.h"
#include "msglogapi.h"
#include "../../Base/Comm/UserInfoClient.h"
#include "log/clog.h"
#include "common.h"
#include "json_util.h"


//extern TMsgLog g_stMsgLog;
//extern CSpeedPosServer g_cOrderServer;

INT32 CProcPushSysBillTask::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
{
    BEGIN_LOG(__func__);
    CDEBUG_LOG("------------process begin----------");
    INT32 iRet = 0;

    //test
//    tars::TC_Parsepara parse;
//    parse.load(mapInput);
//    string parse_str = parse.tostr();
//    CDEBUG_LOG("parse_str = [%s]",parse_str.c_str());
    //end
	gettimeofday(&m_stStart, NULL);
	Reset();

	if ( !m_bInited )
	{
		snprintf( m_szErrMsg, sizeof(m_szErrMsg), "Not Inited" );
		m_iRetCode = -1;
		return m_iRetCode;
	}

	//获取请求
	iRet = FillReq(mapInput);
	if ( iRet != 0 )
	{
		m_iRetCode = iRet;
		BuildResp(outbuf, outlen);
		return m_iRetCode;
	}

	//校验请求
	iRet = CheckInput();
	if ( iRet != 0 )
	{
		snprintf( m_szErrMsg, sizeof(m_szErrMsg),
				  "CProcPushSysBillTask::Execute CheckInput Failed.Ret[%d]", iRet );
		m_iRetCode = iRet;
		BuildResp(outbuf, outlen);
		return m_iRetCode;
	}

	try
	{
		iRet = HandleProcess();
	}
	catch(CTrsExp& e)
	{
		m_stResp.err_code = atoi(e.retcode.c_str());
		m_stResp.err_msg = e.retmsg;
		BuildResp(outbuf, outlen);
		CDEBUG_LOG("------------exception process end----------");
		return m_stResp.err_code;
	}
	catch(...)
	{
		m_stResp.err_code = -1;
		m_stResp.err_msg = "Unknown Exception";
		BuildResp(outbuf, outlen);
		CDEBUG_LOG("------------exception process end----------");
		return m_stResp.err_code;
	}

	m_stResp.err_code = 0;
	m_stResp.err_msg = RESP_SUCCUSS_MSG;
	BuildResp( outbuf, outlen );
	CDEBUG_LOG("------------process end----------");
	return m_iRetCode;

}

/**
* brief: 
* param:
* out: 
* return: succ:RET_SUCCESS_UNIX
* */
INT32 CProcPushSysBillTask::createSubProcess(clib_mysql* pDbCon)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	pid_t pid;

	for (int i = 0; i < 10; i++)
	{
		pid = fork();
		if (pid < 0)
		{
			CERROR_LOG("CProcPushSysBillTask  createSubProcess fork child process .\n");
		}
		else if (pid == 0)
		{
			/*iRet = g_cOrderServer.PushSysPaymentBill(pDbCon, m_stReq.iBmId, m_iBillBeginTime, m_iBillEndTime, i, pBillShopFile, pBillChannelFile);
			if (iRet < 0)
			{
				snprintf(m_szErrMsg, sizeof(m_szErrMsg), "PushSysPaymentBill failed ! "
					"Ret[%d] Err[%s]",
					iRet, g_cOrderServer.GetErrorMessage());
				CERROR_LOG("PushSysPaymentBill failed! "
					"Ret[%d] Err[%s].\n",
					iRet, g_cOrderServer.GetErrorMessage());
				return -10;
			}*/
			exit(0);
		}
		else
		{
			vecPid.push_back(pid);
		}
	}
	return iRet;
}

INT32 CProcPushSysBillTask::CalcEffectiveTimeBill()
{
	BEGIN_LOG(__func__);
	if (m_stReq.sInputTime.empty())
	{
		m_iBillBeginTime = GetYesterday();
		m_iBillEndTime   = m_iBillBeginTime + (24 * 3600) - 1;
	}
	else
	{
		char szBuf[50] = { 0 };
		snprintf(szBuf, sizeof(szBuf), "%s %s", m_stReq.sInputTime.c_str(), "00:00:00");
		std::string strDate = szBuf;
		m_iBillBeginTime = toUnixTime(strDate);
		m_iBillEndTime = m_iBillBeginTime + (24 * 3600) - 1;
	}

	CDEBUG_LOG("CalcEffectiveTimeBill ! "
		"iBillBeginTime:[%d] iBillEndTime:[%d].\n",
		m_iBillBeginTime,
		m_iBillEndTime);

	return 0;
}

INT32 CProcPushSysBillTask::CheckBillFill()
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	CBillBusiConfig* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;
	string temppath;
	if (0 == strcmp(m_stReq.sPayChannel.c_str(), SYS_API_PAY_CHANNEL))
	{
		sMchBillSrcFillName = mainConfig.sMchBillPath  + m_stReq.sBmId + "/" + SRC_PATH
				+ mainConfig.sMchBillSrcFilePrefix + "_" + getSysDate(m_iBillBeginTime) + ".csv";
		sMchBillEncFileName = mainConfig.sMchBillPath  + m_stReq.sBmId + "/" + ENCRYPT_PATH
				+ mainConfig.sMchBillEncFilePrefix + "_" + getSysDate(m_iBillBeginTime) + ".csv";

		sChannelBillSrcFillName = mainConfig.sChannelBillPath  + m_stReq.sBmId + "/" + SRC_PATH
				+ mainConfig.sChannelBillSrcFilePrefix + "_" + getSysDate(m_iBillBeginTime) + ".csv";
		sChannelBillEncFileName = mainConfig.sChannelBillPath  + m_stReq.sBmId + "/" + ENCRYPT_PATH
				+ mainConfig.sChannelBillEncFilePrefix + "_" + getSysDate(m_iBillBeginTime) + ".csv";
		CDEBUG_LOG("CheckBillFill ! "
			"sMchBillSrcFillName:[%s] sMchBillEncFileName:[%s] sChannelBillSrcFillName[%s] sChannelBillEncFileName[%s].\n",
			sMchBillSrcFillName.c_str(), sMchBillEncFileName.c_str(),
			sChannelBillSrcFillName.c_str(), sChannelBillEncFileName.c_str());
		//../client/speedpos_bill/data/bill/1/src/
		if (access(sMchBillSrcFillName.c_str(), F_OK) == 0)  //存在，返回0 ;不存在或无权限，返回 -1
		{
			if (remove(sMchBillSrcFillName.c_str()) != 0)
			{
				CERROR_LOG("CheckBillFill  remove sMchBillSrcFillName[%s] Err.\n", sMchBillSrcFillName.c_str());
			}
		}
		else
		{
			//判断目录是否存在
			temppath = mainConfig.sMchBillPath + m_stReq.sBmId + "/" + SRC_PATH;
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
		//../client/speedpos_bill/data/bill/1/src/   ---channel跟src同一个目录
		if (access(sChannelBillSrcFillName.c_str(), F_OK) == 0)
		{
			if (remove(sChannelBillSrcFillName.c_str()) != 0)
			{
				CERROR_LOG("CheckBillFill  remove sChannelBillSrcFillName[%s] Err.\n", sChannelBillSrcFillName.c_str());
			}
		}

		if (access(sChannelBillEncFileName.c_str(), F_OK) == 0)
		{
			if (remove(sChannelBillEncFileName.c_str()) != 0)
			{
				CERROR_LOG("CheckBillFill  remove sChannelBillEncFileName[%s] Err.\n", sChannelBillEncFileName.c_str());
			}
		}

		std::string sMchBillSrcFillPath = mainConfig.sMchBillPath +  m_stReq.sBmId +  "/" + SRC_PATH  + mainConfig.sMchBillSrcFilePrefix;
		std::string sChannelBillSrcFillPath = mainConfig.sChannelBillPath + m_stReq.sBmId + "/" + SRC_PATH + mainConfig.sChannelBillSrcFilePrefix;
		pBillShopFile = new CBillFile(sMchBillSrcFillPath.c_str(), m_iBillBeginTime);
		iRet = pBillShopFile->raw("%s\n", BILL_FILE_TABLE_HEAD);

		if (0 != iRet)
		{
			CERROR_LOG("pBillShopFile raw Fail! "
				"Ret[%d] .\n",
				iRet);
			return -1;
		}
		pBillChannelFile = new CBillFile(sChannelBillSrcFillPath.c_str(), m_iBillBeginTime);
		iRet = pBillChannelFile->raw("%s\n", CHANNEL_BILL_FILE_TABLE_HEAD);
		if (0 != iRet)
		{
			CERROR_LOG("pBillShopFile raw Fail! "
				"Ret[%d] .\n",
				iRet);
			return -1;
		}
		CDEBUG_LOG("BillShopFile FileName[%s] ! "
			"BillChannelFile FileName[%s] .\n",
			pBillShopFile->GetFileName().c_str(),
			pBillChannelFile->GetFileName().c_str());
	}
//	else if (m_stReq.sPayChannel ==  WX_API_PAY_CHANNEL)
//	{
//		sWxBillSrcFillName = mainConfig.sWXBillPath + m_stReq.sBmId + "/" + SRC_PATH + mainConfig.sWXBillSrcPrefix + "_" + getSysDate(m_iBillBeginTime);
//		sWxBillEncFileName = mainConfig.sWXBillPath + m_stReq.sBmId + "/" + ENCRYPT_PATH + mainConfig.sWXBillEncPrefix + "_" + getSysDate(m_iBillBeginTime);
//
//		CDEBUG_LOG("CheckBillFill ! "
//			"sWxBillSrcFillName:[%s] sWxBillEncFileName:[%s] .\n",
//			sWxBillSrcFillName.c_str(), sWxBillEncFileName.c_str());
//		if (access(sWxBillSrcFillName.c_str(), F_OK) == 0)
//		{
//			if (remove(sWxBillSrcFillName.c_str()) != 0)
//			{
//				CERROR_LOG("CheckBillFill  remove sWxBillSrcFillName[%s] Err.\n", sWxBillSrcFillName.c_str());
//			}
//		}
//		if (access(sWxBillEncFileName.c_str(), F_OK) == 0)
//		{
//			if (remove(sWxBillEncFileName.c_str()) != 0)
//			{
//				CERROR_LOG("CheckBillFill  remove sWxBillEncFileName[%s] Err.\n", sWxBillEncFileName.c_str());
//			}
//		}
//
//		std::string sWxBillSrcFillPath = mainConfig.sWXBillPath + m_stReq.sBmId + "/" + SRC_PATH + mainConfig.sWXBillSrcPrefix;
//		pBillWxFile = new CBillFile(sWxBillSrcFillPath.c_str(), m_iBillBeginTime);
//	}
//	else if (0 == strcmp(m_stReq.sPayChannel.c_str(), ALI_API_PAY_CHANNEL))
//	{
//		sAliBillSrcFillName = mainConfig.sAliBillPath + m_stReq.sBmId + "/" + SRC_PATH +
//				mainConfig.sAliBillSrcPrefix + "_" + getSysDate(m_iBillBeginTime) + ".csv";
//		sAliBillEncFileName = mainConfig.sAliBillPath + m_stReq.sBmId + "/" + ENCRYPT_PATH +
//				mainConfig.sAliBillEncPrefix + "_" + getSysDate(m_iBillBeginTime) + ".csv";
//
//		CDEBUG_LOG("CheckBillFill ! "
//			"sAliBillSrcFillName:[%s] sAliBillEncFileName:[%s] .\n",
//			sAliBillSrcFillName.c_str(), sAliBillEncFileName.c_str());
//		if (access(sAliBillSrcFillName.c_str(), F_OK) == 0)
//		{
//			if (remove(sAliBillSrcFillName.c_str()) != 0)
//			{
//				CERROR_LOG("CheckBillFill  remove sAliBillSrcFillName[%s] Err.\n", sAliBillSrcFillName.c_str());
//			}
//		}
//		if (access(sAliBillEncFileName.c_str(), F_OK) == 0)
//		{
//			if (remove(sAliBillEncFileName.c_str()) != 0)
//			{
//				CERROR_LOG("CheckBillFill  remove sAliBillEncFileName[%s] Err.\n", sAliBillEncFileName.c_str());
//			}
//		}
//	}




	return 0;
}

INT32 CProcPushSysBillTask::HandleProcess()
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;

	if (0 != CalcEffectiveTimeBill()) return -10;
	if (0 != CheckBillFill()) return -20;
	if (0 == strcmp(m_stReq.sPayChannel.c_str(), SYS_API_PAY_CHANNEL))
	{
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
				iRet = PushSysPaymentBill(pDbCon, iDbIndex,bFirstTable);
				if (iRet < 0)
				{
					CERROR_LOG("PushSysPaymentBill failed! Ret[%d] Err[%s].\n",iRet, "fail");
					throw(CTrsExp(iRet,"fail"));

				}
				bFirstTable = false;
			}

		}
		// 生成退款对账单文件
		iRet = PushSysRefundBill();
		if (iRet < 0)
		{
			CERROR_LOG("PushSysPaymentBill failed! Ret[%d] Err[%s].\n",iRet, "fail");
			throw(CTrsExp(iRet,"fail"));
		}
	}
	else if (0 == strcmp(m_stReq.sPayChannel.c_str(), WX_API_PAY_CHANNEL))
	{
		// 生成WX对账单文件
		iRet = PushWxBill();
		if (iRet < 0)
		{
			CERROR_LOG("PushWxBill failed! Ret[%d] Err[%s].\n",iRet, "PushWxBill fail");
			throw(CTrsExp(iRet,"PushWxBill fail"));
		}
	}
	else if (0 == strcmp(m_stReq.sPayChannel.c_str(), ALI_API_PAY_CHANNEL))
	{
		// 生成ali对账单文件
		CBillBusiConfig* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
		STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;
		std::string strPath = mainConfig.sAliBillPath + m_stReq.sBmId + "/" + SRC_PATH;
		std::string strDetailSuffix = mainConfig.sAliDetailSuffix;
		std::string strGatewayUrl = mainConfig.sAliGateWayUrl;
		iRet = PushAliBill(strPath,strDetailSuffix,strGatewayUrl);
		if (iRet < 0)
		{
			CERROR_LOG("PushAliBill failed! Ret[%d] Err[%s].\n",iRet, "PushAliBill fail");
			throw(CTrsExp(iRet,"PushAliBill fail"));
		}
	}
	
	return 0;
}
/**
* brief:
* param:
* out:
* return: succ:RET_SUCCESS_UNIX
**/
INT32 CProcPushSysBillTask::checkSubProcess()
{
	//阻塞等待
	for (vector<pid_t>::iterator itPid = vecPid.begin(); itPid != vecPid.end();)
	{
		int iStatus = 0;
		pid_t child = waitpid(*itPid, &iStatus, 0);
		if (child == 0)
		{
			itPid++;
			continue;
		}
		else
		{
			vecPid.erase(itPid);
		}
	}
	return 0;
}

/*
 * 解析出请求结构
 * 成功返回0 失败返回-1
 */
INT32 CProcPushSysBillTask::FillReq( NameValueMap& mapInput)
{
    BEGIN_LOG(__func__);
    m_stReq.Reset();

    FETCH_INT_VALUE( mapInput, m_stReq.stHead.iVersion, "VER", -1, "CProcPushSysBillTask::GetReq Field[VER] invalid" );
    FETCH_INT_VALUE( mapInput, m_stReq.stHead.iCmd, "CMD", -2, "CProcPushSysBillTask::GetReq Field[CMD] invalid" );
    FETCH_STRING_VALUE_EX_EX( mapInput, m_stReq.stHead.szUserIP, "SPBILL_CREATE_IP", "0.0.0.0" );
    FETCH_INT_VALUE( mapInput, m_stReq.stHead.iSrc, "SRC", -3, "CProcPushSysBillTask::GetReq Field[SRC] invalid" );
    FETCH_STRING_VALUE_EX_EX( mapInput, m_stReq.szVersion, "VERSION", "");

    FETCH_STRING_STD(mapInput, m_stReq.sBmId, "BM_ID", -5, "CProcPushSysBillTask::GetReq Field[BM_ID] invalid");
	FETCH_STRING_STD(mapInput, m_stReq.sPayChannel, "PAY_CHANNEL", -6, "CPayTradeCreateTask::GetReq Field[PAY_CHANNEL] invalid");
	FETCH_STRING_STD_EX_EX(mapInput, m_stReq.sInputTime, "INPUT_TIME", "");

    snprintf( m_szLogMessage, sizeof(m_szLogMessage),
              "CProcPushSysBillTask : ver[%d] cmd[%d] ip[%s] src[%d] version[%s] "
			  "sBmId[%s] sPayChannel[%s] InputTime[%s]",
              m_stReq.stHead.iVersion, m_stReq.stHead.iCmd, m_stReq.stHead.szUserIP, m_stReq.stHead.iSrc, m_stReq.szVersion,
              m_stReq.sBmId.c_str(), m_stReq.sPayChannel.c_str(), m_stReq.sInputTime.c_str());

    return 0;
}

INT32 CProcPushSysBillTask::CheckInput()
{
    
    return 0;
}

INT32 CProcPushSysBillTask::PushSysPaymentBill(clib_mysql* pDbCon,const int& iDbIndex,bool bFirstTab)

{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	std::vector<std::string> vecMchIds;
	char szOrderTableName[128] = { 0 };
	char szOrderChannelTableName[128] = { 0 };
	std::string strDataBaseSuffix = getSysDate(m_iBillBeginTime).substr(0, 6);
	snprintf(szOrderTableName, sizeof(szOrderTableName), "pay_order_db_%s.t_order_%d", strDataBaseSuffix.c_str(), iDbIndex);
	snprintf(szOrderChannelTableName, sizeof(szOrderChannelTableName), "pay_order_db_%s.t_order_channel_%d", strDataBaseSuffix.c_str(), iDbIndex);

	CDEBUG_LOG("ordertable_name[%s] orderchanneltable_name[%s]!", szOrderTableName, szOrderChannelTableName);

	iRet = m_stTransFlowDao.GetOrderTableAllMchIdSql(*pDbCon,
			szOrderTableName,
		m_stReq.sBmId,
		m_iBillBeginTime,
		m_iBillEndTime,
		vecMchIds,bFirstTab);
	if (iRet < 0)
	{
		CERROR_LOG("GetOrderTableAllMchIdSql failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
		throw(CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage()));
	}

	//计算商户
	for (std::vector<std::string>::iterator iter = vecMchIds.begin(); iter != vecMchIds.end(); ++iter)
	{
		std::vector<OrderFlowData> vecOrderFlowDatas;
		iRet = m_stTransFlowDao.GetOrderTableFlowDataSql(*pDbCon,
				szOrderTableName,
			m_stReq.sBmId,
			*iter,
			m_iBillBeginTime,
			m_iBillEndTime,
			vecOrderFlowDatas);
		//CDEBUG_LOG("iBmId[%d] iMchId[%d] iBeginTime[%d] iEndTime[%d]!", iBmId, *iter, iBeginTime, iEndTime);
		if (iRet < 0)
		{
			CERROR_LOG("GetOrderTableAllMchIdSql failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
			throw(CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage()));
		}

		iRet = CreateSysPaymentBillFill(pDbCon, szOrderChannelTableName, vecOrderFlowDatas);
		if (iRet < 0)
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg), "GetOrderTableFlowDataSql failed ! "
				"Ret[%d] Err[%s]",
				iRet, m_stTransFlowDao.GetErrorMessage());
			CERROR_LOG("GetOrderTableFlowDataSql failed! "
				"Ret[%d] Err[%s].\n",
				iRet, m_stTransFlowDao.GetErrorMessage());
			return -20;
		}
	}
	return 0;
}

INT32 CProcPushSysBillTask::CreateSysPaymentBillFill(clib_mysql* pDbCon,
										std::string strOrderChannelTableName,
										std::vector<OrderFlowData>& vecOrderFlowDatas)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	char szLinBuf[4096] = { 0 };
	char szChBuf[4096] = { 0 };
	for (std::vector<OrderFlowData>::iterator itOrder = vecOrderFlowDatas.begin(); itOrder != vecOrderFlowDatas.end(); ++itOrder)
	{

		memset(szLinBuf, 0x0, sizeof(szLinBuf));
		snprintf(szLinBuf, sizeof(szLinBuf), "`%s,`%s,`%s,`%s,`%s,`%s,`%s,`%s,"
			"`%s,`%s,`%d,`%d,`%d,`%d,`%s,"
			"`%s,`%s,`%d,`%d,`%d,`%d,`%d,`%s,`%s,`%d",
			m_stReq.sBmId.c_str(),getSysTime(itOrder->pay_time).c_str(), itOrder->order_no.c_str(), itOrder->out_trade_no.c_str(),
			itOrder->transaction_id.c_str(), itOrder->mch_id.c_str(), itOrder->channel_id.c_str(), itOrder->pay_channel.c_str(),
			itOrder->trade_type.c_str(), "SUCCESS", itOrder->total_fee, itOrder->total_commission, itOrder->shop_amount, 0, "",
			"", "", itOrder->payment_profit, itOrder->channel_profit, itOrder->bm_profit, itOrder->service_profit,0,
			itOrder->fee_type.c_str(),itOrder->sub_body.c_str(),itOrder->shop_calc_rate);

		//生成支付交易对账单
		CDEBUG_LOG("order order_no[%s] szBuf[%s]!", itOrder->order_no.c_str(), szLinBuf);
		pBillShopFile->raw("%s\n", szLinBuf);//所有商户的

		//查询订单号流水
		std::vector<OrderChannelFlowData> vecOrderChannelFlowDatas;

		iRet = m_stTransFlowDao.GetOrderChannelTableFlowDataSql(*pDbCon,
																strOrderChannelTableName,
																itOrder->order_no,
																vecOrderChannelFlowDatas);
		if (iRet < 0)
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg), "GetOrderChannelTableFlowDataSql failed ! "
				"Ret[%d] Err[%s]",
				iRet, m_stTransFlowDao.GetErrorMessage());
			CERROR_LOG("GetOrderTableFlowDataSql failed! "
				"Ret[%d] Err[%s].\n",
				iRet, m_stTransFlowDao.GetErrorMessage());
			return -10;
		}
		//交易时间,平台订单号,平台商户号,渠道ID,支付渠道,支付类型,交易状态,
		//交易金额,退款金额,平台退款单号,该渠道结算费率,该渠道佣金
		for (size_t i = 0; i < vecOrderChannelFlowDatas.size(); ++i)
		{
			memset(szChBuf, 0x0, sizeof(szChBuf));
			snprintf(szChBuf, sizeof(szChBuf), "`%s,`%s,`%s,`%s,`%s,`%s,`%s,`%s,"
				"`%d,`%d,`%s,`%d,`%d",
				m_stReq.sBmId.c_str(),getSysTime(vecOrderChannelFlowDatas[i].pay_time).c_str(), vecOrderChannelFlowDatas[i].order_no.c_str(), vecOrderChannelFlowDatas[i].mch_id.c_str(), vecOrderChannelFlowDatas[i].channel_id.c_str(), itOrder->pay_channel.c_str(), itOrder->trade_type.c_str(), "SUCCESS",
				vecOrderChannelFlowDatas[i].total_fee, 0, "", vecOrderChannelFlowDatas[i].channel_profit_rate, vecOrderChannelFlowDatas[i].channel_profit);
			//CDEBUG_LOG("orderchannel order_no[%s] szOrderChBuf[%s]!", itOrder->order_no.c_str(), szLinBuf);
			pBillChannelFile->raw("%s\n", szChBuf);//商户所属渠道的
		}
	}
	return 0;
}

/**
* brief:
* param:
* out:
* return: succ:RET_SUCCESS_UNIX
* */
INT32 CProcPushSysBillTask::PushSysRefundBill()
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	char szLinBuf[4096] = { 0 };
	char szChBuf[4096] = { 0 };
	clib_mysql* pShopDb = Singleton<CSpeedPosConfig>::GetInstance()->GetShopDB();

	/*获取昨天起始时间  结束时间*/
	std::vector<OrderRefundFlowData> vecOrderRefundFlowDatas;

	iRet = m_stTransFlowDao.GetOrderRefundTableFlowDataSql(*pShopDb,
															m_stReq.sBmId,
															REFUND_ORDER_STATUS_SUCC,
															m_iBillBeginTime,
															m_iBillEndTime,
															vecOrderRefundFlowDatas);
	if (iRet < 0)
	{
		CERROR_LOG("GetOrderTableAllMchIdSql failed! Ret[%d] Err[%s].\n",iRet, m_stTransFlowDao.GetErrorMessage());
		throw(CTrsExp(iRet,m_stTransFlowDao.GetErrorMessage()));
	}

	for (std::vector<OrderRefundFlowData>::iterator itRefund = vecOrderRefundFlowDatas.begin(); itRefund != vecOrderRefundFlowDatas.end(); ++itRefund)
	{
		memset(szLinBuf, 0x0, sizeof(szLinBuf));
		snprintf(szLinBuf, sizeof(szLinBuf), "`%s,`%s,`%s,`%s,`%s,`%s,`%s,`%s,"
			"`%s,`%s,`%d,`%d,`%d,`%d,`%s,"
			"`%s,`%s,`%d,`%d,`%d,`%d,`%d,`%s,`%s,`%d",
			m_stReq.sBmId.c_str(),getSysTime(itRefund->refund_time).c_str(), itRefund->order_no.c_str(), "", "", itRefund->mch_id.c_str(), itRefund->channel_id.c_str(),
			itRefund->pay_channel.c_str(),itRefund->trade_type.c_str(), "REFUND", itRefund->total_fee, itRefund->refund_total_commission,
			itRefund->refund_shop_amount, itRefund->refund_fee, itRefund->refund_no.c_str(),itRefund->out_refund_no.c_str(),itRefund->refund_id.c_str(),
			itRefund->refund_payment_profit, itRefund->refund_channel_profit, itRefund->refund_bm_profit, itRefund->refund_service_profit,0,
			itRefund->fee_type.c_str(),itRefund->sub_body.c_str(),itRefund->shop_calc_rate);

		//CDEBUG_LOG("refund order_no[%s] refund_no[%s] szLinBuf[%s]!", itRefund->order_no.c_str(), itRefund->refund_no.c_str(), szLinBuf);
		pBillShopFile->raw("%s\n", szLinBuf);
		//交易时间,平台订单号,平台商户号,渠道ID,支付渠道,支付类型,交易状态,
		//交易金额,退款金额,平台退款单号,该渠道结算费率,该渠道佣金,
		std::vector<OrderRefundChannelFlowData>  vecOrderRefundChannelDatas;
		iRet = m_stTransFlowDao.GetOrderRefundChannelTableFlowDataSql(*pShopDb,
																	  itRefund->refund_no,
																      vecOrderRefundChannelDatas);
		if (iRet < 0)
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg), "GetOrderRefundChannelTableFlowDataSql failed ! "
				"Ret[%d] Err[%s]",
				iRet, m_stTransFlowDao.GetErrorMessage());
			CERROR_LOG("GetOrderRefundChannelTableFlowDataSql failed! "
				"Ret[%d] Err[%s].\n",
				iRet, m_stTransFlowDao.GetErrorMessage());
			return -10;
		}
		for (size_t i = 0; i < vecOrderRefundChannelDatas.size(); ++i)
		{
			snprintf(szChBuf, sizeof(szChBuf), "`%s,`%s,`%s,`%s,`%s,`%s,`%s,`%s,"
				"`%d,`%d,`%s,`%d,`%d",
				m_stReq.sBmId.c_str(),getSysTime(vecOrderRefundChannelDatas[i].refund_time).c_str(), itRefund->order_no.c_str(), vecOrderRefundChannelDatas[i].mch_id.c_str(), vecOrderRefundChannelDatas[i].channel_id.c_str(), itRefund->pay_channel.c_str(), itRefund->trade_type.c_str(), "REFUND",
				0, vecOrderRefundChannelDatas[i].refund_fee, vecOrderRefundChannelDatas[i].refund_no.c_str(), 0, vecOrderRefundChannelDatas[i].refund_channel_profit);
			//CDEBUG_LOG("refundchannel order_no[%s] refund_no[%s] szLinBuf[%s]!", itRefund->order_no.c_str(), vecOrderRefundChannelDatas[i].refund_no.c_str(), szChBuf);
			pBillChannelFile->raw("%s\n", szChBuf);//商户所属渠道的
		}
	}

	return 0;
}


/**
* brief:
* param:
* out:
* return: succ:RET_SUCCESS_UNIX
* */
int CProcPushSysBillTask::PushWxBill()
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;
	time_t tNow = time(NULL);

	clib_mysql* pBillDb = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
	CBillBusiConfig* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;
	CMySQL m_mysql;
	ostringstream sqlss;
	SqlResultMapVector outmapVector;

	std::string strBillDate = getSysDate(m_iBillBeginTime);
	std::string strNonceStr = toString(tNow);

    sqlss.str("");
    sqlss <<"select gateway_id,appid,mch_id,gateway_key "
    	  <<" from "<<SHOP_DB<<"."<<SHOP_GATEWAY
		  <<" where bm_id ='"<<m_stReq.sBmId<<"' and pay_channel ='"<<m_stReq.sPayChannel<<"' and status = '1';";

    iRet = m_mysql.QryAndFetchResMVector(*pBillDb,sqlss.str().c_str(),outmapVector);
    if(iRet == 1)
    {
    	//有数据
    	for(size_t i = 0;i < outmapVector.size();i++)
    	{
    		sWxBillSrcFillName.clear();
    		StringMap paramMap;
    		paramMap.insert(StringMap::value_type("appid", outmapVector[i]["appid"]));
    		paramMap.insert(StringMap::value_type("mch_id", outmapVector[i]["mch_id"]));
    		paramMap.insert(StringMap::value_type("bill_date", strBillDate));
    		paramMap.insert(StringMap::value_type("bill_type", "ALL"));
    		paramMap.insert(StringMap::value_type("nonce_str", strNonceStr));

    		std::string strBody;
    		std::string strKey = outmapVector[i]["gateway_key"];
    		std::string strGatewayId = outmapVector[i]["gateway_id"];
    		int retcode = m_speedposserver.SendRequestWxapi(strKey, WX_DOWNLOAD_URL, paramMap, strBody);
    		if (retcode)
    		{
    			CERROR_LOG("err_code [%d] \n", retcode);
    			continue;
    		}
    		CDEBUG_LOG("WX res BODY:[%s]",strBody.c_str());
    		//WX返回 如果有对账数据，则返回具体交易数据，如果有错误则返回XML格式报文
    		tinyxml2::XMLDocument wx_rsp_doc;
    		if (tinyxml2::XML_SUCCESS != wx_rsp_doc.Parse(strBody.c_str(), strBody.size()))
    		{
    			sWxBillSrcFillName = mainConfig.sWXBillPath + m_stReq.sBmId + "/" + SRC_PATH + mainConfig.sWXBillSrcPrefix + "_"
    					+ strGatewayId + "_" + getSysDate(m_iBillBeginTime) + ".csv";

    			tars::TC_File::save2file(sWxBillSrcFillName,strBody);
    			CDEBUG_LOG("write BillWxFile [%s] finished!",sWxBillSrcFillName.c_str());
    			continue;
    		}
    		tinyxml2::XMLElement * xmlElement = wx_rsp_doc.FirstChildElement("xml");
    		if (NULL == xmlElement)
    		{
    			CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_WX_PASE_RETRUN_DATA, errMap[ERR_WX_PASE_RETRUN_DATA].c_str());
    		}

    	}
    }

//	StringMap paramMap;
//
//	if (!p_wxpay_cfg->strAppid.empty())
//
//		paramMap.insert(StringMap::value_type("appid", p_wxpay_cfg->strAppid));
//
//	if (!p_wxpay_cfg->strMchId.empty())
//		paramMap.insert(StringMap::value_type("mch_id", p_wxpay_cfg->strMchId));
//	paramMap.insert(StringMap::value_type("bill_date", strBillDate));
//	paramMap.insert(StringMap::value_type("bill_type", "ALL"));
//	paramMap.insert(StringMap::value_type("nonce_str", strNonceStr));
//
//	std::string strKey = p_wxpay_cfg->strPaySignKey;
//
//	std::string strBody;
//	int retcode = m_speedposserver.SendRequestWxapi(strKey, WX_DOWNLOAD_URL, paramMap, strBody);
//	if (retcode){
//		CERROR_LOG("err_code [%d] \n", retcode);
//	}
//	tinyxml2::XMLDocument wx_rsp_doc;
//	if (tinyxml2::XML_SUCCESS != wx_rsp_doc.Parse(strBody.c_str(), strBody.size()))
//	{
//		pBillWxFile->_write(strBody.c_str(), strBody.length());
//		CDEBUG_LOG("write BillWxFile [%s] finished!",pBillWxFile->GetFileName().c_str());
//		return 0;
//	}
//
//	tinyxml2::XMLElement * xmlElement = wx_rsp_doc.FirstChildElement("xml");
//	if (NULL == xmlElement)
//	{
//		CERROR_LOG("err_code [%d] err_code_msg [%s]\n", ERR_WX_PASE_RETRUN_DATA, errMap[ERR_WX_PASE_RETRUN_DATA].c_str());
//		return ERR_WX_PASE_RETRUN_DATA;
//	}
//	const char* pwxrsp_return_code = m_speedposserver.GetXmlField(xmlElement, "return_code");
//	const char* pwxrsp_return_msg = m_speedposserver.GetXmlField(xmlElement, "return_msg");
//	const char* pwxrsp_result_code = m_speedposserver.GetXmlField(xmlElement, "result_code");
//
//	CDEBUG_LOG("wx rep :code = [%s],msg =[%s],result_code=[%s]",pwxrsp_return_code,pwxrsp_return_msg,pwxrsp_result_code);
//	if (0 == strcmp(pwxrsp_return_code, "FAIL"))
//	{
//		pBillWxFile->_open();
//	}

	return 0;
}

/**
* brief:
* param:
* out:
* return: succ:RET_SUCCESS_UNIX
* */
int CProcPushSysBillTask::PushAliBill(std::string& strPath, std::string& detailsuffix,std::string& strGatewayUrl)
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;

	clib_mysql* pBillDb = Singleton<CSpeedPosConfig>::GetInstance()->GetBillDB();
	CBillBusiConfig* pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;
	CMySQL m_mysql;
	ostringstream sqlss;
	SqlResultMapVector outmapVector;
	std::string downloadurl;

	std::string strBillDate = toDate(getSysDate(m_iBillBeginTime));

    sqlss.str("");
    sqlss <<"select gateway_id,appid,mch_id,gateway_key,pub_key,pri_key "
    	  <<" from "<<SHOP_DB<<"."<<SHOP_GATEWAY
		  <<" where bm_id ='"<<m_stReq.sBmId<<"' and pay_channel ='"<<m_stReq.sPayChannel<<"' and status = '1';";

    iRet = m_mysql.QryAndFetchResMVector(*pBillDb,sqlss.str().c_str(),outmapVector);
    if(iRet == 1)
    {
    	//有数据
    	for(size_t i = 0;i < outmapVector.size();i++)
    	{
    		sAliBillSrcFillName.clear();
    		sAliBillSrcFillName = mainConfig.sAliBillPath + m_stReq.sBmId + "/" + SRC_PATH + mainConfig.sAliBillSrcPrefix + "_"
    				+ outmapVector[i]["gateway_id"] + "_" + getSysDate(m_iBillBeginTime) + ".csv";

    		//先把pub_key 内容转存到文件中
    		std::string strPubPem = "../client/speedpos_bill/conf/cert/abc/alipay_formal_public.pem";
    		std::string strPriPem = "../client/speedpos_bill/conf/cert/abc/alipay_formal_private.pem";
    		tars::TC_File::save2file(strPubPem,outmapVector[i]["pub_key"]);
    		tars::TC_File::save2file(strPriPem,outmapVector[i]["pri_key"]);

    		OpenapiClient openapiClient(outmapVector[i]["appid"],
    				strPriPem,
    			strGatewayUrl,
    			OpenapiClient::default_charset,
				strPubPem);

    		JsonMap contentMap;
    		contentMap.insert(JsonMap::value_type(JsonType("bill_type"), JsonType("trade")));
    		contentMap.insert(JsonMap::value_type(JsonType("bill_date"), JsonType(strBillDate)));

    		/** 调用Openapi网关 **/
    		JsonMap rspMap;
    		//如果有扩展参数，则按如下方式传入
    		rspMap = openapiClient.invoke(ALI_DOWNLOAD_METHOD, contentMap);
    		if (!rspMap.size())
    		{
    			CDEBUG_LOG("rspMap no contents!");
    			continue;
    		}
    		//解析支付宝返回数据
    		JsonMap::const_iterator iter;
    		if ((iter = rspMap.find("code")) != rspMap.end())
    		{
    			if (0 != strcmp(rspMap["code"].toString().c_str(), "10000"))
    			{
    				CDEBUG_LOG("err_code [%s] err_code_msg [%s]\n", rspMap["code"].toString().c_str(), rspMap["msg"].toString().c_str());
    				continue;
    			}
    		}
    		iter = rspMap.find("bill_download_url");
    		if (iter != rspMap.end())
    		{
    			downloadurl = iter->second.toString();
    		}

    		int iSeparateIndex = downloadurl.find("?");
    		std::string downloadFileName;
    		std::string strReq = downloadurl.substr(iSeparateIndex + 1, downloadurl.size() - iSeparateIndex - 1);
    		CDEBUG_LOG("downloadurl = %s strPath = %s strReq = %s \n", downloadurl.c_str(), strPath.c_str(), strReq.c_str());
    		UrlParamMap reqMap;
    		reqMap.parseUrl(strReq.c_str());
    		UrlParamMap::iterator iterUrlMap;
    		iterUrlMap = reqMap.find("downloadFileName");
    		if (iterUrlMap != reqMap.end())
    		{
    			downloadFileName = iterUrlMap->second;
    		}
    		std::string strFile = strPath + downloadFileName;
    		CDEBUG_LOG("downloadFileName = %s strFile = %s \n", downloadFileName.c_str(), strFile.c_str());
    		m_speedposserver.SendRequestDownload(downloadurl, strFile);
    		char szZipBuf[256] = { 0 };
    		snprintf(szZipBuf, sizeof(szZipBuf), "unzip %s -d %s\n", strFile.c_str(), strPath.c_str());
    		system(szZipBuf);
    		std::string strCsvFix = downloadFileName.substr(0, downloadFileName.find("."));

    		char szToUtf[128] = { 0 };
    		code_convert("UTF-8","GB2312", detailsuffix.c_str(), strlen(detailsuffix.c_str()), szToUtf, sizeof(szToUtf));

    		std::string strTargetName = strCsvFix + szToUtf;
    		CDEBUG_LOG("strCsvFix = %s strTargetName = %s \n", strCsvFix.c_str(), strTargetName.c_str());
    		if(!tars::TC_File::isFileExist(strPath+strTargetName))
    		{
    			CDEBUG_LOG("detail file not exist [%s]",strTargetName.c_str());
    			continue;
    			//throw(CTrsExp(ERR_DETAIL_FILE_NOT_FOUND,errMap[ERR_DETAIL_FILE_NOT_FOUND]));
    		}
    		//换文件名的同时进行转码 gbk -> utf-8
    		char szToBuf[256] = { 0 };
    		snprintf(szToBuf, sizeof(szToBuf), "iconv -f GBK -t UTF-8 %s%s -o %s\n", strPath.c_str(), strTargetName.c_str(), sAliBillSrcFillName.c_str());
    		CDEBUG_LOG("szToBuf [%s]", szToBuf);
    		system(szToBuf);

    	}
    }

//	std::string downloadurl = "";
//	const CBillBusiConfig::AliPayCfg* p_alipay_cfg = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig()->
//		GetAliPayCfg(m_stReq.sBmId);
//
//	if(NULL == p_alipay_cfg)
//	{
//		CERROR_LOG("p_wxpay_cfg bussiness config not found");
//		return -1;
//	}

//	std::string strBillDate = toDate(getSysDate(m_iBillBeginTime));
//	OpenapiClient openapiClient(p_alipay_cfg->strAppid,
//		p_alipay_cfg->strAppPrivCert,
//		strGatewayUrl,
//		OpenapiClient::default_charset,
//		p_alipay_cfg->strAppCert);
//	JsonMap contentMap;
//	contentMap.insert(JsonMap::value_type(JsonType("bill_type"), JsonType("trade")));
//	contentMap.insert(JsonMap::value_type(JsonType("bill_date"), JsonType(strBillDate)));
//
//	/** 调用Openapi网关 **/
//	JsonMap rspMap;
//	//如果有扩展参数，则按如下方式传入
//	rspMap = openapiClient.invoke(ALI_DOWNLOAD_METHOD, contentMap);
//	if (!rspMap.size()) return ERR_ALI_PAY_RSA_VERIFY;
//
//	JsonMap::const_iterator iter;
//	if ((iter = rspMap.find("code")) != rspMap.end())
//	{
//		if (0 != strcmp(rspMap["code"].toString().c_str(), "10000"))
//		{
//			CERROR_LOG("err_code [%s] err_code_msg [%s]\n", rspMap["code"].toString().c_str(), rspMap["msg"].toString().c_str());
//			return ERR_ALI_PAY_UNIFIEDORDER_ERR_MSG;
//		}
//	}
//	/** 解析支付宝返回报文 **/
//	iter = rspMap.find("bill_download_url");
//	if (iter != rspMap.end())
//	{
//		downloadurl = iter->second.toString();
//	}
//
//	//SendRequestDownload(downloadurl, strPath);
//	int iSeparateIndex = downloadurl.find("?");
//	std::string downloadFileName;
//	std::string strReq = downloadurl.substr(iSeparateIndex + 1, downloadurl.size() - iSeparateIndex - 1);
//	CDEBUG_LOG("downloadurl = %s strPath = %s strReq = %s \n", downloadurl.c_str(), strPath.c_str(), strReq.c_str());
//	UrlParamMap reqMap;
//	reqMap.parseUrl(strReq.c_str());
//	UrlParamMap::iterator iterUrlMap;
//	iterUrlMap = reqMap.find("downloadFileName");
//	if (iterUrlMap != reqMap.end())
//	{
//		downloadFileName = iterUrlMap->second;
//	}
//	std::string strFile = strPath + downloadFileName;
//	CDEBUG_LOG("downloadFileName = %s strFile = %s \n", downloadFileName.c_str(), strFile.c_str());
//	m_speedposserver.SendRequestDownload(downloadurl, strFile);
//	char szZipBuf[256] = { 0 };
//	snprintf(szZipBuf, sizeof(szZipBuf), "unzip %s -d %s\n", strFile.c_str(), strPath.c_str());
//	system(szZipBuf);
//	std::string strCsvFix = downloadFileName.substr(0, downloadFileName.find("."));
//
//	char szToUtf[128] = { 0 };
//	code_convert("UTF-8","GB2312", detailsuffix.c_str(), strlen(detailsuffix.c_str()), szToUtf, sizeof(szToUtf));
//
//	std::string strTargetName = strCsvFix + szToUtf;
//	CDEBUG_LOG("strCsvFix = %s strTargetName = %s \n", strCsvFix.c_str(), strTargetName.c_str());
//	if(!tars::TC_File::isFileExist(strPath+strTargetName))
//	{
//		CERROR_LOG("detail file not exist [%s]",strTargetName.c_str());
//		throw(CTrsExp(ERR_DETAIL_FILE_NOT_FOUND,errMap[ERR_DETAIL_FILE_NOT_FOUND]));
//	}
//	//换文件名的同时进行转码 gbk -> utf-8
//	char szToBuf[256] = { 0 };
//	snprintf(szToBuf, sizeof(szToBuf), "iconv -f GBK -t UTF-8 %s%s -o %s\n", strPath.c_str(), strTargetName.c_str(), strToName.c_str());
//	CDEBUG_LOG("szToBuf [%s]", szToBuf);
//	system(szToBuf);
	return 0;
}

void CProcPushSysBillTask::BuildResp( CHAR** outbuf, INT32& outlen )
{
    CHAR szResp[ MAX_RESP_LEN ];
    //CHAR szResult[ MAX_RESP_LEN ];
	JsonMap jsonRsp; 
	jsonRsp.insert(JsonMap::value_type(JsonType("retcode"), JsonType((double)m_stResp.err_code)));
	jsonRsp.insert(JsonMap::value_type(JsonType("retmsg"), JsonType(m_stResp.err_msg)));
	if (!m_stResp.sReturnContent.empty()) jsonRsp.insert(JsonMap::value_type(JsonType("Content"), JsonType(m_stResp.sReturnContent)));
	std::string resContent = JsonUtil::objectToString(jsonRsp);

	snprintf(szResp, sizeof(szResp), //remaincount=1
		"%s\r\n",
		resContent.c_str());

	outlen = strlen(szResp);
	*outbuf = (char*)malloc(outlen);
	memcpy(*outbuf, szResp, outlen);

	CDEBUG_LOG("Rsp :[%s]",szResp);
	CDEBUG_LOG("-----------time userd:[%d ms] ---------",SpeedTime());

}

void CProcPushSysBillTask::LogProcess()
{
   
}
