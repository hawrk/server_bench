/*
 * CProcSettleCallback.cpp
 *
 *  Created on: 2017年4月19日
 *      Author: hawrkchen
 */


#include <sys/time.h>
#include "tools.h"
//#include "msglogapi.h"
#include "../../Base/Comm/UserInfoClient.h"
#include "log/clog.h"
#include "common.h"
#include "CProcSettleCallback.h"
#include "DBPool.h"
#include "json_util.h"
#include "network.h"
#include <unistd.h>
#include "util/tc_file.h"
#include <sys/mman.h>

//extern TMsgLog g_stMsgLog;
extern CSpeedPosServer g_cOrderServer;

CProcSettleCallback::CProcSettleCallback()
{
	pSettleFille = NULL;
	pBillBusConfig = NULL;
	pBankResultfile = NULL;

}

CProcSettleCallback::~CProcSettleCallback()
{
  //Dector
	if(pSettleFille != NULL)
	{
		delete pSettleFille;
		pSettleFille = NULL;
	}
	if(pBankResultfile != NULL)
	{
		delete pBankResultfile;
		pBankResultfile = NULL;
	}
	if(pBillBusConfig != NULL)
	{
		delete pBillBusConfig;
		pBillBusConfig = NULL;
	}
}



INT32 CProcSettleCallback::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
{
    BEGIN_LOG(__func__);
    CDEBUG_LOG("------------process begin----------");
    INT32 iRet = 0;

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
                  "CProcSettleCallback::Execute CheckInput Failed.Ret[%d]", iRet );
        m_iRetCode = iRet;
        BuildResp(outbuf, outlen);
        return m_iRetCode;
    }

	iRet = HandleProcess();
    if (0 != iRet) {
        m_iRetCode = iRet;
        CERROR_LOG("CProcSettleCallback HandleProcess Err! Msg[%s] Ret[%d].",
			GetErrorMessage(), iRet);
//        snprintf(m_szErrMsg, sizeof(m_szErrMsg), "CProcSettleCallback HandleProcess Failed.Ret[%d] "
//			"Msg[%s].", iRet, GetErrorMessage());
		m_stResp.err_code = iRet;
		m_stResp.err_msg = GetErrorMessage();
        BuildResp(outbuf, outlen);
        CDEBUG_LOG("------------exception process end----------");
        return m_iRetCode;
    }
	m_stResp.err_code = 0;
	m_stResp.err_msg = RESP_SUCCUSS_MSG;
    BuildResp( outbuf, outlen );

    CDEBUG_LOG("------------process end----------");

    return m_iRetCode;
}


INT32 CProcSettleCallback::CallSettle()
{
	BEGIN_LOG(__func__);

	int iRet = 0;
	pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
	STBillSrvMainConf mainConfig = pBillBusConfig->mainConfig;

	const CBillBusiConfig::BankAttr* p_bank_attr = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig()
		->GetBankAttrCfg(m_stReq.sBmId);

	std::string RemittancePath = mainConfig.sRemittancePath;

	std::string settle_file_path = RemittancePath  + m_stReq.sBmId + "/result/";
	std::string bank_result_path = RemittancePath + m_stReq.sBmId + "/bank/";

	pSettleFille = new CBillFile(settle_file_path.c_str(), m_stReq.sFileName.c_str());
	pBankResultfile = new CBillFile(bank_result_path.c_str(),m_stReq.sFileName.c_str());
	CDEBUG_LOG("settlefilepath:[%s]",pSettleFille->GetFileName().c_str());

	//校验一下bank下面的银行编号目录是否存在
	if(access(bank_result_path.c_str(),F_OK) != 0) //目录不存在，则新建一个
	{
		if(mkdir(bank_result_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
		{
			CERROR_LOG("  create dir fail! sMchBillSrcFillName[%s] Err.\n", bank_result_path.c_str());
		}
	}

	if(p_bank_attr->strEncrypt == "1") //需要加密
	{
		DES_Decrypt(pSettleFille->GetFileName().c_str(),pBankResultfile->GetFileName().c_str());
	}
	else
	{
		tars::TC_File::copyFile(pSettleFille->GetFileName(),pBankResultfile->GetFileName(),true);
		//CopyFile(pSettleFille->GetFileName().c_str(),pBankResultfile->GetFileName().c_str()); //覆盖
	}

	//读取文件内容
	void *memory = NULL;
	vector<std::string> vecBill;
	int fd = open(pBankResultfile->GetFileName().c_str(), O_RDONLY);
	if (fd < 0)
	{
		CERROR_LOG("file open [%s] error\n", pBankResultfile->GetFileName().c_str());
		return -40;
	}
	int file_length = lseek(fd, 1, SEEK_END);
	memory = mmap(NULL, file_length, PROT_READ, MAP_SHARED, fd, 0);
	split_ex((char *)memory, '\n', vecBill);

	for(vector<std::string>::iterator it = vecBill.begin(); it != vecBill.end();it++)
	{
		vector<string> vStr;
		TRemitBill remitBill;
		int status = 0;
		//boost::split(vStr,(*it), boost::is_any_of( "," ), boost::token_compress_on );//token_compress_on此标志为多个空值时，压缩为一个值
		boost::split(vStr,(*it), boost::is_any_of( "," ));

		CDEBUG_LOG("parse line :[%s]",(*it).c_str());

		if(vStr.size() < 14)  //第一行为汇总数据的话，则跳过
			continue;

//			for(vector<std::string>::iterator subit = vStr.begin();subit != vStr.end();subit ++)
//			{
//				CDEBUG_LOG("sub evec:[%s]",(*subit).c_str());
//			}
		//序号,交易日期,平台编号,平台类型,平台名称,开户名,银行卡号,卡类型(0:对私;1:对公),银行类型,网点号,结算金额(单位:元),结算时间,描述
		remitBill.account_id = vStr[2];
		remitBill.sType = vStr[3];
		remitBill.sPayTime = vStr[1];
		remitBill.sRemitTime = getSysDate();
		status = (vStr[13] == "0000") ? 1:-2;
		remitBill.sPayRemark = vStr[14];

		iRet = g_cOrderServer.CallUpdateSettleLogApi(m_stReq.sBmId, m_stReq.sPayChannel, remitBill, status);
		if (iRet < 0)
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg), "CallUpdateSettleLogApi failed ! "
				"Ret[%d]",
				iRet);
			CERROR_LOG("CallUpdateSettleLogApi failed! "
				"Ret[%d].\n",
				iRet);
			return -3060;
		}
	}
	//处理完成后，删除源目录的加密文件
	std::string settle_file = settle_file_path + m_stReq.sFileName;
	remove(settle_file.c_str());


//	SOrderInfoRsp orderInfo;
//	INT32 iRet = g_cOrderServer.getsppClent().CallOrderQuery(m_stReq.sOrderNo, orderInfo);
//	CDEBUG_LOG("retcode = [%d],ret_msg =[%s]",orderInfo.result.ret_code,orderInfo.result.ret_msg.c_str());
//	//
//	if(orderInfo.result.ret_code < 0)
//	{
//		snprintf(m_szErrMsg, sizeof(m_szErrMsg),"Query Order Fail[%s]",orderInfo.result.ret_msg.c_str());
//		return -1;
//	}
//
//	CDEBUG_LOG("sOrder_no =[%s],sOrder_state = [%d]",orderInfo.order_no.c_str(),orderInfo.order_status);
//	if(orderInfo.order_no.empty()|| 0 == orderInfo.order_no.length())
//	{
//		return -1;
//	}
//	if(orderInfo.order_status == 2|| orderInfo.order_status == 5)  //SUCCESS 或　REFUND 不允许操作
//	{
//		return -2;
//	}

	return 0;
}

INT32 CProcSettleCallback::HandleProcess()
{
	BEGIN_LOG(__func__);
	INT32 iRet = 0;

	iRet = CallSettle();
    if ( iRet != 0 )
    {
    	return -1;
    }

	return 0;
}


/*
 * 解析出请求结构
 * 成功返回0 失败返回-1
 */
INT32 CProcSettleCallback::FillReq( NameValueMap& mapInput)
{
    BEGIN_LOG(__func__);
    m_stReq.Reset();

    FETCH_INT_VALUE( mapInput, m_stReq.stHead.iVersion, "VER", -1, "CProcSettleCallback::GetReq Field[VER] invalid" );
    FETCH_INT_VALUE( mapInput, m_stReq.stHead.iCmd, "CMD", -2, "CProcSettleCallback::GetReq Field[CMD] invalid" );
    FETCH_STRING_VALUE_EX_EX( mapInput, m_stReq.stHead.szUserIP, "SPBILL_CREATE_IP", "0.0.0.0" );
    FETCH_INT_VALUE( mapInput, m_stReq.stHead.iSrc, "SRC", -3, "CProcSettleCallback::GetReq Field[SRC] invalid" );
    FETCH_STRING_VALUE_EX_EX( mapInput, m_stReq.szVersion, "VERSION", "");

    FETCH_STRING_STD(mapInput, m_stReq.sBmId, "BM_ID", -5, "CProcBillContrastTask::GetReq Field[BM_ID] invalid");
	FETCH_STRING_STD(mapInput, m_stReq.sPayChannel, "PAY_CHANNEL", -6, "CPayTradeCreateTask::GetReq Field[PAY_CHANNEL] invalid");
	FETCH_STRING_STD(mapInput,m_stReq.sFileName,"FILE_NAME", -7,"CProcBillContrastTask::GetReq Field[FILE_NAME] invalid");


    snprintf( m_szLogMessage, sizeof(m_szLogMessage),
              "CProcSettleCallback : ver[%d] cmd[%d] ip[%s] src[%d] version[%s] "
			  "bm_id:[%s] pay_channel:[%s],filename=[%s]",
              m_stReq.stHead.iVersion, m_stReq.stHead.iCmd, m_stReq.stHead.szUserIP, m_stReq.stHead.iSrc, m_stReq.szVersion,
              m_stReq.sBmId.c_str(), m_stReq.sPayChannel.c_str(),m_stReq.sFileName.c_str());

    return 0;
}

INT32 CProcSettleCallback::CheckInput()
{

    return 0;
}

void CProcSettleCallback::BuildResp( CHAR** outbuf, INT32& outlen )
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
	CDEBUG_LOG("------------time userd:[%d ms]---------",SpeedTime());

}

void CProcSettleCallback::LogProcess()
{
	//CDEBUG_LOG("~~~~~~~end %s~~~~~~~",__FUNCTION__ );
}

