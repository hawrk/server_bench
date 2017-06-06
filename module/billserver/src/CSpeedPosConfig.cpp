/*
 * COrderConfig.cpp
 *
 *  Created on: 2010-5-27
 *      Author: rogeryang
 */

#include "../../Base/include/xmlParser.h"
#include "log/clog.h"
#include "tools.h"
#include "CMTClient.h"
#include "CSpeedPosConfig.h"
#include "CBillBusiConfig.h"


const char * REPORTER_ID = "ec00054";

INT32 CSpeedPosConfig::LoadConfig(const CHAR *pchConfig)
{
    XMLNode xMainNode=XMLNode::openFileHelper( pchConfig,"order_config" );

    fprintf( stderr, "load clog...\n" );
    XMLNode xClogNode = xMainNode.getChildNode( "clog" );
    snprintf( m_szLogDir, sizeof(m_szLogDir), "%s",
            xClogNode.getAttribute("logdir") );
    snprintf( m_szLogPrefix, sizeof(m_szLogPrefix), "%s",
            xClogNode.getAttribute("logprefix") );
    m_iLogLevel = atoi( xClogNode.getAttribute("loglevel") );
    m_iLogSize = atoi( xClogNode.getAttribute("logsize") );

    //order_server
    fprintf( stderr, "load core_cfg...\n" );
    XMLNode xCoreCfgNode = xMainNode.getChildNode( "core_cfg" );
    m_sTokenPrefix = xCoreCfgNode.getAttribute("token_prefix");
    m_uiTokenLen = atoll( xCoreCfgNode.getAttribute("token_len") );
    m_sSerialPrefix = xCoreCfgNode.getAttribute("serial_prefix");
    m_uiSerialLen = atoll( xCoreCfgNode.getAttribute("serial_len") );
    //m_uiExpireTime = atoll( xCoreCfgNode.getAttribute("expire_time") );
	m_strGrouponLockFile = xCoreCfgNode.getAttribute("groupon_lock_file");
    m_strMerchantIdLockFile = xCoreCfgNode.getAttribute("merchant_id_lock_file");
    m_strUinLockFileFilterSvr = xCoreCfgNode.getAttribute("uin_lock_file_filter_svr");
    m_sSecureToken = xCoreCfgNode.getAttribute("secure_token");

	XMLNode xTTC;
	INT32 iUseL5 = 0;
    //id_gen_server
    fprintf( stderr, "load pay_server...\n" );
	FETCH_SERVER_INFO("trade_server", m_stTradeServer);

	//settle_gen_server
	fprintf(stderr,"load settle_server...\n");
	FETCH_SERVER_INFO("settle_server",m_stSettleServer);


#ifdef _SYBASE

	fprintf(stderr, "load shop db...\n");
	XMLNode xDbNode = xMainNode.getChildNode("sybase_shop_db");
	std::string shophost = xDbNode.getAttribute("host");
	std::string shopUser = xDbNode.getAttribute("user");
	std::string shopPwd = xDbNode.getAttribute("pwd");
	m_stShopDbName = xDbNode.getAttribute("dbname");

	fprintf(stderr, "load bill db...\n");
	XMLNode xBillDbNode = xMainNode.getChildNode("sybase_bill_db");
	std::string sBillhost = xBillDbNode.getAttribute("host");
	std::string sBillUser = xBillDbNode.getAttribute("user");
	std::string sBillPwd = xBillDbNode.getAttribute("pwd");
	m_stBillDbName = xBillDbNode.getAttribute("dbname");

	pBillDB = new CSyBase;
	pBillDB->init(sBillhost.c_str(), sBillUser.c_str(), sBillPwd.c_str(),m_stBillDbName.c_str());

#else

	fprintf(stderr, "load shop db...\n");
	XMLNode xDbNode = xMainNode.getChildNode("shop_db");
	std::string shopIp = xDbNode.getAttribute("ip");
	int shop_port = atoi(xDbNode.getAttribute("port"));
	std::string shopUser = xDbNode.getAttribute("user");
	std::string shopPwd = xDbNode.getAttribute("pwd");
	m_stShopDbName = xDbNode.getAttribute("db");

	fprintf(stderr, "load bill db...\n");
	XMLNode xBillDbNode = xMainNode.getChildNode("bill_db");
	std::string sBillIp = xBillDbNode.getAttribute("ip");
	int iBillPort = atoi(xBillDbNode.getAttribute("port"));
	std::string sBillUser = xBillDbNode.getAttribute("user");
	std::string sBillPwd = xBillDbNode.getAttribute("pwd");
	m_stBillDbName = xBillDbNode.getAttribute("db");

	pShopDB = new clib_mysql;
	pShopDB->init(shopIp.c_str(), shop_port, shopUser.c_str(), shopPwd.c_str());

	pBillDB = new clib_mysql;
	pBillDB->init(sBillIp.c_str(), iBillPort, sBillUser.c_str(), sBillPwd.c_str());
#endif
    //notify
   /* fprintf( stderr, "load notify...\n" );
    XMLNode xNotify = xMainNode.getChildNode("notify");
    m_kNotifyKey = atoll( xNotify.getAttribute("ipckey") );*/
	

    //singleLimitAtLeast
    XMLNode xSingleLimitNode = xMainNode.getChildNode("singleLimitAtLeast");
    int iSingleLimitNode = atoi(xSingleLimitNode.getAttribute("underlyingRule"));
    fprintf(stderr, "order_config/singleLimitAtLeast/underlyingRule:%d\n", iSingleLimitNode);
    if(1 == iSingleLimitNode)
        m_SingleLimitAtLeastUnderlyingRule = true;
    else
        m_SingleLimitAtLeastUnderlyingRule = false;

	fprintf(stderr, "load pay config and db pool ..\n");
	XMLNode xPayCfgNode = xMainNode.getChildNode("pay_config");
	std::string strCfgPath = xPayCfgNode.getAttribute("confpath");

	//开始加载bill_busi_conf.xml 配置内容
	pBusConfig = new CBillBusiConfig(strCfgPath.c_str());
	
	//
#ifdef _SYBASE

#else
	pDBPool = new CDBPool;
	std::vector<CBillBusiConfig::DbCfg>::iterator it_dbcfg = pBusConfig->dbCfgVec.begin();
	for (; it_dbcfg != pBusConfig->dbCfgVec.end(); ++it_dbcfg)
	{
		pDBPool->AddDBConn(it_dbcfg->db_num, it_dbcfg->type, it_dbcfg->host, it_dbcfg->port, it_dbcfg->user, it_dbcfg->pswd);
	}
#endif


    return 0;
}

void CSpeedPosConfig::PrintConfig()
{
    fprintf( stderr, "------------- Bill Server, Build Time %s:%s------------------\n",
            __DATE__, __TIME__ );
    fprintf( stderr, "core_cfg: token_prefix[%s] token_len[%u] serial_prefix[%s] serial_"
                "len[%u] groupon_lock_file[%s] uin_lock_file[%s] token[%s]\n",
                m_sTokenPrefix.c_str(), m_uiTokenLen, m_sSerialPrefix.c_str(), m_uiSerialLen,
				m_strGrouponLockFile.c_str(), m_strMerchantIdLockFile.c_str(), m_sSecureToken.c_str());


	PRINT_SERVER_INFO("trade_server", m_stTradeServer);
   
}

INT32 CSpeedPosConfig::InitLog()
{
    return c_log_init(m_szLogDir, m_iLogLevel, m_iLogSize, m_szLogPrefix);
}

INT32 CSpeedPosConfig::InitOrderServer(CSpeedPosServer& order)
{
   

    return order.Init(m_stTradeServer,m_stSettleServer);
}

