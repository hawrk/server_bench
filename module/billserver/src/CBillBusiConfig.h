#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <string>
#include "log/clog.h"
#include <list>
#include <map>
#include <vector>
#include <errno.h>
#include "../business/bill_protocol.h"
#include "../../Base/Comm/comm_protocol.h"
#include "url_protocol.h"

class CBillBusiConfig
{
public:
	struct WxPayCfg
	{
		std::string strAppid;
		std::string strMchId;
		std::string strPaySignKey;
		std::string strAppCert;
		std::string strAppPrivCert;
	};

	struct AliPayCfg
	{
		std::string strAppid;
		std::string strPid;
		std::string strPriveKey;
		std::string strPublicKey;
		std::string strAppCert;
		std::string strAppPrivCert;
	};

	//add hawrk 银行编号对应的属性
	struct BankAttr
	{
		std::string strBankType;
		std::string strEncrypt;
		std::string strNotify;
		std::string strUseSftp;
	};

	struct DbCfg
	{
		std::string host;     // 数据库主机IP
		std::string user;     // 数据库用户名
		std::string pswd;     // 数据库密码
		std::string db_name;
		int port;       //端口
		int db_num;  // 
		int type;
		DbCfg() :port(0), db_num(0), type(0){}
	};

public:
   
	std::vector<DbCfg>  dbCfgVec;

	std::vector<DbCfg>  apay_dbCfgVec;

	std::map<std::string,BankAttr> bak_attr;

	std::map<std::string, WxPayCfg> wx_pay_cfgs;

	std::map<std::string, AliPayCfg> ali_pay_cfgs;

	STBillSrvMainConf mainConfig;

	ApayBillSrvMainConf apayMainConf;

public:
	//function definition
	CBillBusiConfig(const char* szFileName);
	~CBillBusiConfig(){};
	
	const WxPayCfg* GetWxPayCfg(std::string bm_id) const;
	const AliPayCfg* GetAliPayCfg(std::string bm_id) const;
	const BankAttr* GetBankAttrCfg(std::string bm_id) const;

    void ReadErrCodeFromFile(NameValueMap& inMap);
private:
    string m_ApayErrFile;    
};
