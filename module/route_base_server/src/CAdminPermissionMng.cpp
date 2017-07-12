/*
 * CAdminPermissionMng.cpp
 *
 *  Created on: 2017年7月11日
 *      Author: hawrkchen
 */
#include "CAdminPermissionMng.h"


INT32 CAdminPermissionMsg::Execute( NameValueMap& mapInput, char** outbuf, int& outlen )
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


	    if(m_InParams["oper_type"] == "1")  //新增
	    {
	    	AddAdminPermissionDB();
	    }
	    else if(m_InParams["oper_type"] == "2")   //编辑
	    {
	    	EditAdminPermissionDB();
	    }
	    else if(m_InParams["oper_type"] == "3")  //查询
	    {
	    	QueryAdminPermissionDB();
	    }
	    else
	    {
	    	CERROR_LOG("Unknow oper_type :[%s] ",m_InParams["oper_type"].c_str());
	    	throw(CTrsExp(ERR_INVALID_PARAMS,"Unknow oper_type!!!"));
	    }

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

void CAdminPermissionMsg::AddAdminPermissionDB()
{
	CDEBUG_LOG("Begin ...");
	int iRet;
	SqlResultSet outMap;

    sqlss.str("");
    sqlss << "select Fperm_id  from "
      	  <<BASE_DB<<"."<<ADMIN_PERMISSION
  		  <<" where Fperm_id = '"<<m_InParams["perm_id"]<<"';";

    iRet = m_mysql.QryAndFetchResMap(*m_pBaseDB,sqlss.str().c_str(),outMap);
    if(iRet == 1)
    {
		CERROR_LOG("t_admin_permission record duplicate!!!");
		throw(CTrsExp(ERR_RECORD_EXIST,"t_admin_permission record duplicate!!!"));
    }

    sqlss.str("");
    sqlss <<"insert into "
    	  <<BASE_DB<<"."<<ADMIN_PERMISSION
		  <<" (Fperm_id,Fperm_name,Fperm_url,Fparent_id,Flevel,Fstatus,Foper_id,Fremark)"
		  <<" values('"<<m_InParams["perm_id"]<<"','"<<m_InParams["perm_name"]<<"','"<<m_InParams["perm_url"]
		  <<"','"<<m_InParams["parent_id"]<<"','"<<m_InParams["level"]<<"','"<<m_InParams["status"]
		  <<"','"<<m_InParams["oper_id"]<<"','"<<m_InParams["remark"]<<"');";

    iRet = m_mysql.Execute(*m_pBaseDB,sqlss.str().c_str());
    if(iRet != 1)
    {
    	CERROR_LOG("insert t_admin_permission fail!!!");
    	throw(CTrsExp(INSERT_DB_ERR,"insert t_admin_permission fail!!!"));
    }

	CDEBUG_LOG("End.");
}


void CAdminPermissionMsg::EditAdminPermissionDB()
{
	CDEBUG_LOG("Begin ...");
	int iRet;
	ostringstream upateSql;
	upateSql.str("");
	if(!m_InParams["perm_name"].empty())
	{
		upateSql <<"Fperm_name = '"<<m_InParams["perm_name"]<<"',";
	}
	if(!m_InParams["perm_url"].empty())
	{
		upateSql <<"Fperm_url = '"<<m_InParams["perm_url"]<<"',";
	}
	if(!m_InParams["parent_id"].empty())
	{
		upateSql <<"Fparent_id = '"<<m_InParams["parent_id"]<<"',";
	}
	if(!m_InParams["level"].empty())
	{
		upateSql <<"Flevel = '"<<m_InParams["level"]<<"',";
	}
	if(!m_InParams["status"].empty())
	{
		upateSql <<"Fstatus = '"<<m_InParams["status"]<<"',";
	}

	sqlss.str("");
	sqlss << "update "
		  <<BASE_DB<<"."<<ADMIN_PERMISSION
		  <<" set "<<upateSql.str()
		  <<"Fupdated_time = now() where Fperm_id ='"<<m_InParams["perm_id"]<<"';";

	iRet = m_mysql.Execute(*m_pBaseDB,sqlss.str().c_str());
	if(iRet != 1)
	{
		CERROR_LOG("update t_admin_permission fail!!!");
		throw(CTrsExp(UPDATE_DB_ERR,"update t_admin_permission fail!!!"));
	}

	CDEBUG_LOG("End.");
}


void CAdminPermissionMsg::QueryAdminPermissionDB()
{
	CDEBUG_LOG("Begin ...");
	int iRet;
	ostringstream sWhereSql;
	SqlResultSet outMap;
	SqlResultMapVector recordVector;
	JsonList m_jsonList;

	int iCnt      = (STOI(m_InParams["page"])-1)* STOI(m_InParams["limit"]);

    if(!m_InParams["perm_id"].empty()&& ! m_InParams["perm_id"].empty())
    {
    	sWhereSql << "and Fperm_id ='"<<m_InParams["perm_id"]<<"' ";
    }
    if(!m_InParams["level"].empty())
    {
    	sWhereSql << "and Flevel = '" << m_InParams["level"] << "' ";
    }

    if(!m_InParams["status"].empty())
    {
    	sWhereSql << "and Fstatus = '"<< m_InParams["status"]<<"' ";
    }

    sqlss.str("");
    sqlss << "select count(*) as count from "
      	  <<BASE_DB<<"."<<ADMIN_PERMISSION
  		  << sWhereSql.str()
		  <<";";
    iRet = m_mysql.QryAndFetchResMap(*m_pBaseDB,sqlss.str().c_str(),outMap);
    if(outMap["count"] == "0")  //没有记录，直接返回
    {
        m_ContentJsonMap.clear();
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outMap["count"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("page"), JsonType(m_InParams["page"])));
    	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType("")));
    	return ;
    }

    sqlss.str("");
    sqlss <<"select Fperm_id,Fperm_name,Fperm_url,Fparent_id,Flevel,"
    		"Fstatus,Foper_id,Fremark,Fcreate_time from "
    	  <<BASE_DB<<"."<<ADMIN_PERMISSION
		  << sWhereSql.str()
		  << " limit " << iCnt << "," << m_InParams["limit"] << ";";

    iRet = m_mysql.QryAndFetchResMVector(*m_pBaseDB,sqlss.str().c_str(),recordVector);
    if(iRet == 1)  //有记录
    {
    	JsonMap BillJsonMap;
    	m_jsonList.clear();
		for(size_t i = 0; i < recordVector.size(); i++)
		{
			BillJsonMap.clear();
			BillJsonMap.insert(JsonMap::value_type(JsonType("perm_id"), JsonType(recordVector[i]["Fperm_id"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("perm_name"), JsonType(recordVector[i]["Fperm_name"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("perm_url"), JsonType(recordVector[i]["Fperm_url"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("parent_id"), JsonType(recordVector[i]["Fparent_id"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("level"), JsonType(recordVector[i]["Flevel"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("status"), JsonType(recordVector[i]["Fstatus"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("oper_id"), JsonType(recordVector[i]["Foper_id"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("remark"), JsonType(recordVector[i]["Fremark"])));
			BillJsonMap.insert(JsonMap::value_type(JsonType("create_time"), JsonType(recordVector[i]["Fcreate_time"])));

			m_jsonList.push_front(JsonList::value_type(BillJsonMap));
		}

    }
    m_ContentJsonMap.clear();
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("total"), JsonType(outMap["count"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("page"), JsonType(m_InParams["page"])));
	m_ContentJsonMap.insert(JsonMap::value_type(JsonType("lists"), JsonType(m_jsonList)));

	CDEBUG_LOG("End.");
}

/*
 * 解析出请求结构
 * 成功返回0 失败返回-1
 */
void CAdminPermissionMsg::FillReq( NameValueMap& mapInput)
{
	NameValueMapIter iter;
	for(iter=mapInput.begin();iter!=mapInput.end();iter++)
	{
		string szName = iter->first;
		transform(szName.begin(), szName.end(), szName.begin(), ::tolower);
		//m_InParams[szName] = getSafeInput(iter->second);
		m_InParams[szName] = iter->second;
	}

}

void CAdminPermissionMsg::CheckInput()
{

	if(m_InParams["ver"].empty() || m_InParams["ver"] != VER)
	{
		CERROR_LOG("ver param[%s] is invalid!",m_InParams["ver"].c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS,"ver param invalid!"));
	}
	//这里卡这个值没什么用，在主函数要根据这个值路由到这个接口来
	//Check::CheckStrParam("cmd", m_InParams["cmd"], 1, 10, true);

	Check::CheckDigitalParam("oper_type",m_InParams["oper_type"],1,3,true);

	if(m_InParams["oper_type"] == "1")
	{
		Check::CheckStrParam("perm_id", m_InParams["perm_id"], 1, 10, true);
		Check::CheckStrParam("perm_name", m_InParams["perm_id"], 1, 20, true);
		Check::CheckStrParam("perm_url", m_InParams["perm_url"], 1, 64, true);
		Check::CheckStrParam("level", m_InParams["level"], 1, 10, true);
		Check::CheckStrParam("status", m_InParams["status"], 1, 10, true);
		Check::CheckStrParam("oper_id", m_InParams["oper_id"], 1, 10, true);
		if(m_InParams["parent_id"].empty())
		{
			m_InParams["parent_id"] = "0";
		}
	}
	else if(m_InParams["oper_type"] == "2")
	{
		Check::CheckStrParam("perm_id", m_InParams["perm_id"], 1, 10, true);
	}

	else if(m_InParams["oper_type"] == "3")
	{
		Check::CheckPage(m_InParams["page"]);
		Check::CheckLimit(m_InParams["limit"]);

		if ( m_InParams["page"].empty() )
		{
			m_InParams["page"] = ITOS(PAGE_DEFAULT);
		}
		if ( m_InParams["limit"].empty() )
		{
			m_InParams["limit"] = ITOS(LIMIT_DEFAULT);
		}
	}


//    for( NameValueMap::iterator it = m_InParams.begin(); it != m_InParams.end(); it++ )
//    {
//        CDEBUG_LOG("key=[%s],value=[%s]", it->first.c_str(), it->second.c_str());
//    }

}

void CAdminPermissionMsg::SetRetParam()
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

void CAdminPermissionMsg::BuildResp( CHAR** outbuf, INT32& outlen )
{
	CDEBUG_LOG("Begin ...");

    CHAR szResp[ MAX_RESP_LEN ];
	JsonMap jsonRsp;

	if(m_RetMap["err_code"].empty())
	{
		m_RetMap["err_code"] = "00";
		m_RetMap["err_msg"]  = "success";
	}
	else
    {
        Singleton<CSpeedPosConfig>::GetInstance()->GetBusiConf()->ReadErrCodeFromFile(m_RetMap);
    }

	jsonRsp.insert(JsonMap::value_type(JsonType("err_code"), JsonType(m_RetMap["err_code"])));
	jsonRsp.insert(JsonMap::value_type(JsonType("err_msg"), JsonType(m_RetMap["err_msg"])));
	jsonRsp.insert(JsonMap::value_type(JsonType("content"), JsonType(m_ContentJsonMap)));

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

void CAdminPermissionMsg::LogProcess()
{
}


