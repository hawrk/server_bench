/*
 * CExceptionProc.cpp
 *
 *  Created on: 2017年8月21日
 *      Author: hawrkchen
 */

#include "CExceptionProc.h"

CExceptionProc::CExceptionProc()
{
	//ctor
}
CExceptionProc::~CExceptionProc()
{
	//dector
}

INT32 CExceptionProc::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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

	    CheckOrderStatus();

	    if(m_InParams["type"] == "1")  //单边账，直接更新处理状态
	    {
			UpdateAbnormalStatus(1);
	    }
	    else if(m_InParams["type"] == "2")  //状态不一致，与订单同步状态
	    {
	    	OrderSync(m_InParams["order_no"]);
	    }
	    else if(m_InParams["type"] == "3")  //金额不符
	    {
	    	UpdateAbnormalStatus(2);
	    }
		else
		{
			CERROR_LOG(" invalid param [type],value [%s]!",m_InParams["type"].c_str());
			throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
		}

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
void CExceptionProc::FillReq( NameValueMap& mapInput)
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

void CExceptionProc::CheckInput()
{

	if(m_InParams["ver"].empty() || m_InParams["ver"] != VER)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}

	Check::CheckStrParam("id", m_InParams["id"], 1, 10);   //异常单ID
	Check::CheckDigitalParam("type", m_InParams["type"], 1, 5,true);  //异常类型
	Check::CheckStrParam("order_no", m_InParams["order_no"], 1, 32);  //支付单号


}

void CExceptionProc::CheckOrderStatus()
{
	int iRet;
	SqlResultSet outMap;

    sqlss.str("");
    if(m_InParams["type"] == "1"||m_InParams["type"] == "2")
    {
        sqlss << "select Fid,Fporcess_status from "
          	  <<ROUTE_BILL_DB<<"."<<BILL_ABNORMAL
    		  <<" where Fid = '"<<m_InParams["id"]<<"'";
    }
    else
    {
        sqlss << "select Fid,Fporcess_status from "
          	  <<ROUTE_BILL_DB<<"."<<BILL_ABNORMAL
    		  <<" where Fpf_order_no = '"<<m_InParams["order_no"]<<"'";
    }

    iRet = m_mysql.QryAndFetchResMap(*m_pBillDB,sqlss.str().c_str(),outMap);

    if(iRet < 0)
    {
		CERROR_LOG("current id :Fid[%s] not found !!...",m_InParams["id"].c_str());
    	throw(CTrsExp(ERR_DATA_NOT_FOUND,"查询无数据！"));
    }

    if(outMap["Fporcess_status"] == "2")  //已处理
    {
		CERROR_LOG("current id :Fid[%s]  has been process !!...",m_InParams["id"].c_str());
    	throw(CTrsExp(UPDATE_DB_ERR,"该笔异常单已处理！"));
    }
}

void CExceptionProc::UpdateAbnormalStatus(int type)
{
	CDEBUG_LOG("-----begin--------");
	int iRet;

	sqlss.str("");

	if(type == 1)
	{
		//更新记账流水和记账状态
		sqlss <<" UPDATE "<<ROUTE_BILL_DB<<"."<<BILL_ABNORMAL
			  <<" SET Fporcess_status = '2',"
			  <<"Fmodify_time = now() where Fid = '"<<m_InParams["id"]<<"';";
	}
	else
	{
		sqlss <<" UPDATE "<<ROUTE_BILL_DB<<"."<<BILL_ABNORMAL
			  <<" SET Fporcess_status = '2',"
			  <<"Fmodify_time = now() where Fpf_order_no = '"<<m_InParams["order_no"]<<"';";
	}



    iRet = m_mysql.Execute(*m_pBillDB,sqlss.str().c_str());
    if(iRet != 1)
    {
    	CERROR_LOG("update t_route_bill_abnormal fail!!!");
    	throw(CTrsExp(UPDATE_DB_ERR,"update t_route_bill_abnormal fail!!!"));
    }


	CDEBUG_LOG("-----end--------");
}

INT32 CExceptionProc::OrderSync(string& order_no)
{
	//int iRet = 0;
	char szRecvBuff[10240] = {0};
	StringMap paramMap;
	StringMap recvMap;
	CSocket* tradeSocket = Singleton<CSpeedPosConfig>::GetInstance()->GetTradeServerSocket();

	paramMap.insert(StringMap::value_type("cmd","1008"));
	paramMap.insert(StringMap::value_type("ver","1.0"));

	JsonMap  bizCJsonMap;
	bizCJsonMap.insert(JsonMap::value_type(JsonType("order_no"), order_no));
	//bizCJsonMap.insert(JsonMap::value_type(JsonType("order_status"), order_status));
	std::string bizContent = JsonUtil::objectToString(bizCJsonMap);
	paramMap.insert(std::make_pair("biz_content", bizContent));

	tradeSocket->SendAndRecvLineEx(paramMap,szRecvBuff,sizeof(szRecvBuff),"\r\n");
	CDEBUG_LOG("recv Msg [%s]",szRecvBuff);

	if(NULL == szRecvBuff||strlen(szRecvBuff) == 0)  //
	{
		CDEBUG_LOG("query bank overflow order fail !!,err_msg[%s]",szRecvBuff);
		throw(CTrsExp(ERR_QUERY_ORDER_STATE,"query bank overflow order fail!!"));
	}

	cJSON* root = cJSON_Parse(szRecvBuff);
	if (0 != strcmp(cJSON_GetObjectItem(root, "ret_code")->valuestring, "0"))
	{
		CERROR_LOG("ret_code:[%s] ret_msg:[%s]\n", cJSON_GetObjectItem(root, "ret_code")->valuestring, cJSON_GetObjectItem(root, "ret_msg")->valuestring);
		return -1;
	}
	CDEBUG_LOG("OrderSync,retcode=[%s]",recvMap["retcode"].c_str());

	//更新处理状态
	UpdateAbnormalStatus(1);

	return 0;

}

void CExceptionProc::SetRetParam()
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

void CExceptionProc::BuildResp( CHAR** outbuf, INT32& outlen )
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

void CExceptionProc::LogProcess()
{
}


