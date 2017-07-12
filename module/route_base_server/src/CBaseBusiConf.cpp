/*
 * CAgentpayBusiConf.cpp
 *
 *  Created on: 2010-5-27
 *      Author: 
 */

#include "../Base/include/xmlParser.h"
#include "clog.h"
#include "tools.h"
#include "CMTClient.h"
#include "../Base/Comm/clib_mysql.h"
#include "CBaseBusiConf.h"



CBaseBusiConf::CBaseBusiConf(const char* szFileName)
{
	//read configuration
	using boost::property_tree::ptree;
	ptree pt;

	try
	{
		read_xml(szFileName, pt);

		BOOST_FOREACH(ptree::value_type& v, pt.get_child("root.sys_cfg"))
		{
			if (v.first == "error_file")
			{
				m_errorFile = v.second.get<std::string>("<xmlattr>.file_path");
			}

            if (v.first == "spdb_front_end_machine")
            {       
                m_spdb_sign_url = v.second.get<std::string>("<xmlattr>.sign_url");
                m_spdb_sign_port = v.second.get<std::string>("<xmlattr>.sign_port");
                m_spdb_nc_svr_ip = v.second.get<std::string>("<xmlattr>.nc_svr_ip");
                m_spdb_nc_svr_port = v.second.get<std::string>("<xmlattr>.nc_svr_port");
                m_spdb_timeout = v.second.get<std::string>("<xmlattr>.timeout");
				m_spdb_masterid = v.second.get<std::string>("<xmlattr>.masterID");
				m_spdb_projectNumber = v.second.get<std::string>("<xmlattr>.projectNumber");
				m_spdb_projectName = v.second.get<std::string>("<xmlattr>.projectName");
				m_spdb_feeNo = v.second.get<std::string>("<xmlattr>.feeNo");
            }

		}

		BOOST_FOREACH(ptree::value_type& v, pt.get_child("root.databases"))
		{
			if (v.first == "dbconfig")
			{
				DbCfg dbCfg; 
				
				dbCfg.host = v.second.get<std::string>("<xmlattr>.ip");
				dbCfg.port = v.second.get<int>("<xmlattr>.port");
				dbCfg.user = v.second.get<std::string>("<xmlattr>.user");
				dbCfg.pswd = v.second.get<std::string>("<xmlattr>.pwd");
				dbCfg.db_name = v.second.get<std::string>("<xmlattr>.dbname");
				dbCfg.db_num = v.second.get<int>("<xmlattr>.dbnum");
				dbCfg.type = v.second.get<int>("<xmlattr>.type");

				dbCfgVec.push_back(dbCfg);
			}
		}		
	}
	catch(const boost::property_tree::xml_parser::xml_parser_error& e)
	{
		CERROR_LOG("Err[%s] errno[%d] \n", e.what(), errno);
		exit(1);
	}
	catch(const std::runtime_error& e)
	{
		CERROR_LOG("Err[%s] errno[%d] \n", e.what(), errno);
		exit(1);
	}

}

void CBaseBusiConf::ReadErrCodeFromFile(NameValueMap& inMap)
{
	//read configuration
	using boost::property_tree::ptree;
	ptree pt;

	string szErrMsg = "";

	read_xml(m_errorFile, pt);

    if(szErrMsg.empty())
	{
		BOOST_FOREACH(ptree::value_type& v, pt.get_child("root.transparent"))
		{
			if (v.first == "tp_code")
			{
                string code = v.second.get<std::string>("<xmlattr>.code");
                if( string::npos != code.find(inMap["err_code"]) )
                {
                    szErrMsg = inMap["err_msg"];
                }
			}
		}
	}
	
    if(szErrMsg.empty())
	{
        BOOST_FOREACH(ptree::value_type& v, pt.get_child("root.agentpay_errcode"))
        {
            if (v.first == "errcode")
            {
                string szGetCode = v.second.get<std::string>("<xmlattr>.code");

                if(szGetCode == inMap["err_code"])
                {
                    szErrMsg = v.second.get<std::string>("<xmlattr>.msg");
                }
            }
        }
	}

	if(szErrMsg.empty())
	{
		BOOST_FOREACH(ptree::value_type& v, pt.get_child("root.default_error"))
		{
			if (v.first == "default_code")
			{
				szErrMsg = v.second.get<std::string>("<xmlattr>.msg");
			}			
		}
	}

    inMap["err_msg"] = szErrMsg;
}


