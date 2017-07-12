/*
 * CAgentpayBusiConf.h
 *
 *  Created on: 2010-5-27
 *      Author: 
 */

#ifndef _C_AGENT_PAY_BUSI_CONFIG_H_
#define _C_AGENT_PAY_BUSI_CONFIG_H_
#include "CObject.h"
#include "../Base/Comm/clib_mysql.h"
#include "DBPool.h"
#include "CSocket.h"
#include "typedef.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <string>
#include "log/clog.h"
#include <list>
#include <map>
#include <vector>
#include <errno.h>
#include "CCommFunc.h"


class CBaseBusiConf : public CObject
{
    public:
	CBaseBusiConf(){

		}
		virtual ~CBaseBusiConf(){

		}

		CBaseBusiConf(const char* szFileName);

		void ReadErrCodeFromFile(NameValueMap& inMap);

public:
	struct DbCfg
	{
		std::string host;	  // 数据库主机IP
		std::string user;	  // 数据库用户名
		std::string pswd;	  // 数据库密码
		std::string db_name;
		int port;		//端口
		int db_num;  // 
		int type;
		DbCfg() :port(0), db_num(0), type(0){}
	};

public:

	string m_spdb_sign_url;
	string m_spdb_sign_port;
	string m_spdb_nc_svr_ip;
	string m_spdb_nc_svr_port;
	string m_spdb_timeout;
	string m_spdb_masterid;
	string m_spdb_projectNumber;
	string m_spdb_projectName;
	string m_spdb_feeNo;

	
	std::vector<DbCfg>  dbCfgVec;

private:
	
	string m_errorFile;
};

#endif /* CORDERCONFIG_H_ */

