#include "CBillBusiConfig.h"
#include <stdio.h>


CBillBusiConfig::CBillBusiConfig(const char* szFileName)
{
	//read configuration
	using boost::property_tree::ptree;
	ptree pt;

	try
	{
		read_xml(szFileName, pt);

		BOOST_FOREACH(ptree::value_type& v, pt.get_child("root.sys_cfg"))
		{
			if (v.first == "pay_apps")
			{
				std::string bm_id = v.second.get<std::string>("<xmlattr>.bm_id");

				BankAttr bankattr;
				bankattr.strBankType = v.second.get<std::string>("<xmlattr>.bank_type");
				bankattr.strEncrypt  = v.second.get<std::string>("<xmlattr>.encrypt");
				bankattr.strNotify   = v.second.get<std::string>("<xmlattr>.settle_notify");
				bankattr.strUseSftp  = v.second.get<std::string>("<xmlattr>.use_sftp");
				bak_attr.insert(std::make_pair(bm_id,bankattr));

				WxPayCfg wxpay_cfg;
				wxpay_cfg.strAppid = v.second.get<std::string>("wxpay.appid");
				wxpay_cfg.strMchId = v.second.get<std::string>("wxpay.mch_id");
				wxpay_cfg.strPaySignKey = v.second.get<std::string>("wxpay.paysign_key");
				wxpay_cfg.strAppCert = v.second.get<std::string>("wxpay.app_cert");
				wxpay_cfg.strAppPrivCert = v.second.get<std::string>("wxpay.app_priv_cert");

				wx_pay_cfgs.insert(std::make_pair(bm_id, wxpay_cfg));

				AliPayCfg alipay_cfg;
				alipay_cfg.strAppid = v.second.get<std::string>("alipay.appid");
				alipay_cfg.strPid = v.second.get<std::string>("alipay.pid");
				alipay_cfg.strPriveKey = v.second.get<std::string>("alipay.private_key");
				alipay_cfg.strPublicKey = v.second.get<std::string>("alipay.public_key");
				alipay_cfg.strAppCert = v.second.get<std::string>("alipay.app_cert");
				alipay_cfg.strAppPrivCert = v.second.get<std::string>("alipay.app_priv_cert");

				ali_pay_cfgs.insert(std::make_pair(bm_id, alipay_cfg));
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
		mainConfig.sFtpIp = pt.get<string>("root.SftpIp");
		mainConfig.iSftpPort = pt.get<int>("root.SftpPort");
		mainConfig.sFtpUser = pt.get<string>("root.SftpUser");
		mainConfig.sFtpPass = pt.get<string>("root.SftpPass");

		mainConfig.sAliGateWayUrl = pt.get<string>("root.ALIGateWayUrl");
		mainConfig.sAliDetailSuffix = pt.get<string>("root.AliDetailSuffix");

		mainConfig.sWXBillPath = pt.get<string>("root.WXBillPath");
		//mainConfig.sWXBillEncPath = pt.get<string>("root.WXBillEncPath");
		mainConfig.sWXBillSrcPrefix = pt.get<string>("root.WXBillSrcPrefix");
		mainConfig.sWXBillEncPrefix = pt.get<string>("root.WXBillEncPrefix");

		//mainConfig.sAliBillSPath = pt.get<string>("root.ALIBillSrcPath");
		//mainConfig.sAliBillEncPath = pt.get<string>("root.ALIBillEncPath");
		mainConfig.sAliBillPath      = pt.get<string>("root.ALIBillPath");
		mainConfig.sAliBillSrcPrefix = pt.get<string>("root.ALIBillSrcPrefix");
		mainConfig.sAliBillEncPrefix = pt.get<string>("root.ALIBillEncPrefix");

		mainConfig.sMchBillSrcFilePrefix = pt.get<string>("root.MchBillSrcFilePrefix");
		//mainConfig.sMchBillSrcPath = pt.get<string>("root.MchBillSrcPath");
		mainConfig.sMchBillEncFilePrefix = pt.get<string>("root.MchBillEncFilePrefix");
		//mainConfig.sMchBillEncPath = pt.get<string>("root.MchBillEncPath");
		mainConfig.sMchBillPath      = pt.get<string>("root.MchBillPath");

		mainConfig.sChannelBillSrcFilePrefix = pt.get<string>("root.ChannelSrcFilePrefix");
		//mainConfig.sChannelBillSrcPath = pt.get<string>("root.ChannelBillSrcPath");
		mainConfig.sChannelBillEncFilePrefix = pt.get<string>("root.ChannelEncFilePrefix");
		//mainConfig.sChannelBillEncPath = pt.get<string>("root.ChannelBillEncPath");
		mainConfig.sChannelBillPath      = pt.get<string>("root.ChannelBillPath");

		mainConfig.sSftpLongRangPath = pt.get<string>("root.SftpLongRangPath");
		//mainConfig.sSftpBillSrcPath = pt.get<string>("root.SftpBillSrcPath");
		//mainConfig.sSftpBillDecPath = pt.get<string>("root.SftpBillDecPath");
		mainConfig.sSftpBillPath      = pt.get<string>("root.SftpBillPath");
		mainConfig.sSftMchBillFilePrefix = pt.get<string>("root.SftMchBillFilePrefix");
		mainConfig.sSftChannelBillFilePrefix = pt.get<string>("root.SftChannelBillFilePrefix");
		mainConfig.sSftWXBillFilePrefix = pt.get<string>("root.SftWXBillFilePrefix");
		mainConfig.sSftAliBillFilePrefix = pt.get<string>("root.SftAliBillFilePrefix");

		mainConfig.sSftpShellPath         = pt.get<string>("root.SftpShellPath");
		mainConfig.sBillFileToDBShellPath = pt.get<string>("root.BillFileToDBShellPath");
		mainConfig.sBillTruncateDBShellPath = pt.get<string>("root.BillTruncateDBShellPath");

		mainConfig.sRemittancePath = pt.get<string>("root.RemittancePath");
		//mainConfig.sRemittanceSrcPath = pt.get<string>("root.RemittanceSrcPath");
		//mainConfig.sRemittanceEncryptPath = pt.get<string>("root.RemittanceEncryptPath");
		mainConfig.sRemittanceSrcFilePrefix = pt.get<string>("root.RemittanceSrcPrefix");
		//mainConfig.sRemittanceResultSrcPath = pt.get<string>("root.RemittanceResultSrcPath");
		mainConfig.sRemittanceResultSrcPrefix = pt.get<string>("root.RemittanceResultSrcPrefix");
		//mainConfig.sRemittanceBankResultPath  = pt.get<string>("root.RemittanceBankResultPath");


		mainConfig.sGetBankNoApiUrl = pt.get<string>("root.GetBankNoApiUrl");
		mainConfig.sAddSettleLogUrl = pt.get<string>("root.AddSettleLogUrl");
		mainConfig.sUpdateSettleLogUrl = pt.get<string>("root.UpdateSettleLogUrl");
		mainConfig.sGetPayFailUrl = pt.get<string>("root.GetPayFailUrl");
		mainConfig.sAddBillContrastUrl = pt.get<string>("root.AddBillContrastUrl");
		mainConfig.sUpdateBillContrastUrl = pt.get<string>("root.UpdateBillContrastUrl");
		mainConfig.sAddExceptionOrderUrl = pt.get<string>("root.AddExceptionOrderUrl");
		mainConfig.sApiKey = pt.get<string>("root.ApiKey");


		mainConfig.sCallNotifyIp = pt.get<string>("root.CallNotifyIp");
		mainConfig.iCallNotifyPort = pt.get<int>("root.CallNotifyPort");

		mainConfig.sCallSettleSerIp = pt.get<string>("root.CallSettleSerIP");
		mainConfig.iCallSettleSerPort = pt.get<int>("root.CallSettleSerPort");

		mainConfig.sOrderDBHost = pt.get<string>("root.OrderDBHost");
		mainConfig.nOrderDBPort = pt.get<int>("root.OrderDBPort");
		mainConfig.sOrderDBUser = pt.get<string>("root.OrderDBUser");
		mainConfig.sOrderDBPass = pt.get<string>("root.OrderDBPass");
		mainConfig.sOrderDBNamePrifix = pt.get<string>("root.OrderDBNamePrifix");


		mainConfig.service_name_no = pt.get<string>("root.service_name_no");
		mainConfig.service_name = pt.get<string>("root.service_name");
		mainConfig.service_name_tag = pt.get<string>("root.service_name_tag");
		mainConfig.service_bank = pt.get<string>("root.service_bank");
		mainConfig.service_bank_account = pt.get<string>("root.service_bank_account");
		for (std::vector<DbCfg>::iterator iter = dbCfgVec.begin(); iter != dbCfgVec.end(); ++iter)
		{
			CDEBUG_LOG("db config db_name[%s] db_num[%d] host[%s]\n", iter->db_name.c_str(), iter->db_num, iter->host.c_str());
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

const CBillBusiConfig::BankAttr* CBillBusiConfig::GetBankAttrCfg(std::string bm_id) const
{
	std::map<std::string, BankAttr>::const_iterator it = bak_attr.find(bm_id);
	if (it != bak_attr.end())
	{
		return &(it->second);
	}
	else
	{
		return NULL;
	}

}
const CBillBusiConfig::WxPayCfg* CBillBusiConfig::GetWxPayCfg(std::string bm_id) const
{
	std::map<std::string, WxPayCfg>::const_iterator it = wx_pay_cfgs.find(bm_id);
	if (it != wx_pay_cfgs.end())
	{
		return &(it->second);
	}
	else
	{
		return NULL;
	}
}

const CBillBusiConfig::AliPayCfg* CBillBusiConfig::GetAliPayCfg(std::string bm_id) const
{
	std::map<std::string, AliPayCfg>::const_iterator  it = ali_pay_cfgs.find(bm_id);
    if (it != ali_pay_cfgs.end())
    {
        return &(it->second);
    }
    else
    {
		return NULL;
    }
}

