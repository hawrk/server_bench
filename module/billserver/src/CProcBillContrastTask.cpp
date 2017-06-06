/*
 * CIdGenTask.cpp
 *
 *  Created on: 2009-6-3
 *      Author: rogeryang
 */

#include <sys/time.h>
#include "CProcBillContrastTask.h"

//extern TMsgLog g_stMsgLog;
extern CSpeedPosServer g_cOrderServer;


CProcBillContrastTask::CProcBillContrastTask()
{
	Reset();

	m_iBillBeginTime = 0;
	m_iBillEndTime = 0;
}


INT32 CProcBillContrastTask::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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
                  "CProcBillContrastTask::Execute CheckInput Failed.Ret[%d]", iRet );
        m_iRetCode = iRet;
        BuildResp(outbuf, outlen);
        return m_iRetCode;
    }
    try
    {
    	if(m_stReq.sStep.empty())
    	{
    		m_stReq.sStep = "1";
    	}

    	CalcEffectiveTimeBill();

    	CBillBusiConfig* pBillBusConfig;

    	pBillBusConfig = Singleton<CSpeedPosConfig>::GetInstance()->GetBillBusiConfig();
    	const CBillBusiConfig::BankAttr* p_bank_attr = pBillBusConfig->GetBankAttrCfg(m_stReq.sBmId);

    	CBankFactory bankfactory;
    	CBillContrastBase *contrastbase = NULL;

    	contrastbase = bankfactory.CreateBankFactory(p_bank_attr->strBankType);

    	if(NULL == contrastbase)
    	{
    		CERROR_LOG("unknown bank type!!!!!!");
			throw CTrsExp(ERR_BANK_TYPE,errMap[ERR_BANK_TYPE]);
    	}

    	auto_ptr<CBillContrastBase> billcontrast(contrastbase);


    	switch (atoi(m_stReq.sStep.c_str()))
    	{
    		//完整 流程，不要加break
    	case 1:
        	billcontrast->CheckBillFill(m_stReq,m_iBillBeginTime);
        	billcontrast->BillFileDownLoad(m_stReq,m_iBillBeginTime);
        	billcontrast->LoadBillFlowToDB(m_stReq,m_iBillBeginTime);

    	case 2:
    		billcontrast->ProcBillComparison(m_stReq,m_iBillBeginTime,m_iBillEndTime);

    	case 3:
    		billcontrast->GetRemitBillData(m_stReq,m_iBillBeginTime,m_iBillEndTime);
    		billcontrast->ProRemitBillProcess(m_stReq,m_iBillBeginTime);

    	case 4:
    		billcontrast->UpdateBillStatus(m_stReq,m_iBillBeginTime);
    		break;
		default:
			throw CTrsExp(ERR_PARAM_INVALID,errMap[ERR_PARAM_INVALID]);
    	}
    }
    catch(CTrsExp& e)
    {
    	if(atoi(e.retcode.c_str()) == ERR_BILL_FILE_EXIST)
    	{
			m_stResp.err_code = atoi(e.retcode.c_str());
			m_stResp.err_msg = "对账文件已存在！";
    	}
    	else
    	{
    		m_stResp.err_code = atoi(e.retcode.c_str());
    		m_stResp.err_msg = e.retmsg;
    		g_cOrderServer.CallUpdateBillContrastApi(m_stReq.sBmId, m_stReq.sPayChannel, getSysDate(m_iBillBeginTime),
    			0, "", -1);
    	}
		BuildResp(outbuf, outlen);
		CDEBUG_LOG("------------exception process end----------");
		return m_stResp.err_code;
    }
    catch(...)
    {
		m_stResp.err_code = -1;
		m_stResp.err_msg = "Unknown Exception";
		g_cOrderServer.CallUpdateBillContrastApi(m_stReq.sBmId, m_stReq.sPayChannel, getSysDate(m_iBillBeginTime),
			0, "", -1);
		BuildResp(outbuf, outlen);
		CDEBUG_LOG("------------exception process end----------");
		return m_stResp.err_code;
    }
    if(iRet < 0)
    {
		g_cOrderServer.CallUpdateBillContrastApi(m_stReq.sBmId, m_stReq.sPayChannel, getSysDate(m_iBillBeginTime),
			0, "", -1);
		m_stResp.err_code = iRet;
		m_stResp.err_msg = errMap[iRet].empty() ? "对账失败" : errMap[iRet];
		BuildResp( outbuf, outlen );
		CDEBUG_LOG("------------exception process end----------");
		return m_stResp.err_code;
    }

	m_stResp.err_code = 0;
	m_stResp.err_msg = RESP_SUCCUSS_MSG;
    BuildResp( outbuf, outlen );
    CDEBUG_LOG("------------ process end----------");

    return m_iRetCode;
}


INT32 CProcBillContrastTask::CalcEffectiveTimeBill()
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


//int CProcBillContrastTask::FileMmap(const std::string& strFilePath, vector<std::string>& vecBill)
//{
//	void *memory = NULL;
//	int file_length = 0;
//	int fd = open(strFilePath.c_str(), O_RDONLY);
//	if (fd < 0)
//	{
//		printf("file open [%s] error\n", strFilePath.c_str());
//		return -40;
//	}
//	file_length = lseek(fd, 1, SEEK_END);
//	memory = mmap(NULL, file_length, PROT_READ, MAP_SHARED, fd, 0);
//	split_ex((char *)memory, '\n', vecBill);
//	close(fd);
//	munmap(memory, file_length);
//	return 0;
//}

/*
 * 解析出请求结构
 * 成功返回0 失败返回-1
 */
INT32 CProcBillContrastTask::FillReq( NameValueMap& mapInput)
{
    BEGIN_LOG(__func__);
    m_stReq.Reset();

    FETCH_INT_VALUE( mapInput, m_stReq.stHead.iVersion, "VER", -1, "CProcBillContrastTask::GetReq Field[VER] invalid" );
    FETCH_INT_VALUE( mapInput, m_stReq.stHead.iCmd, "CMD", -2, "CProcBillContrastTask::GetReq Field[CMD] invalid" );
    FETCH_STRING_VALUE_EX_EX( mapInput, m_stReq.stHead.szUserIP, "SPBILL_CREATE_IP", "0.0.0.0" );
    FETCH_INT_VALUE( mapInput, m_stReq.stHead.iSrc, "SRC", -3, "CProcBillContrastTask::GetReq Field[SRC] invalid" );
    FETCH_STRING_VALUE_EX_EX( mapInput, m_stReq.szVersion, "VERSION", "");

    FETCH_STRING_STD(mapInput, m_stReq.sBmId, "BM_ID", -5, "CProcBillContrastTask::GetReq Field[BM_ID] invalid");
	FETCH_STRING_STD(mapInput, m_stReq.sPayChannel, "PAY_CHANNEL", -6, "CPayTradeCreateTask::GetReq Field[PAY_CHANNEL] invalid");
	FETCH_STRING_STD_EX_EX(mapInput, m_stReq.sInputTime, "INPUT_TIME", "");

	FETCH_STRING_STD_EX_EX(mapInput,m_stReq.sStep,"STEP","");

    snprintf( m_szLogMessage, sizeof(m_szLogMessage),
              "CProcBillContrastTask : ver[%d] cmd[%d] ip[%s] src[%d] version[%s] "
			  "iBmId[%s] sPayChannel[%s] InputTime[%s]",
              m_stReq.stHead.iVersion, m_stReq.stHead.iCmd, m_stReq.stHead.szUserIP, m_stReq.stHead.iSrc, m_stReq.szVersion,
			  m_stReq.sBmId.c_str(), m_stReq.sPayChannel.c_str(), m_stReq.sInputTime.c_str());

    CDEBUG_LOG("ReqMsg [%s]",m_szLogMessage);

    return 0;
}

INT32 CProcBillContrastTask::CheckInput()
{
	
    return 0;
}

void CProcBillContrastTask::BuildResp( CHAR** outbuf, INT32& outlen )
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
	CDEBUG_LOG("-----------time userd:[%d ms]---------",SpeedTime());

}

void CProcBillContrastTask::LogProcess()
{
   
}
