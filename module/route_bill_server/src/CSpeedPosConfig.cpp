/*
 * COrderConfig.cpp
 *
 *  Created on: 2010-5-27
 *      Author: rogeryang
 */

#include "../Base/include/xmlParser.h"
#include "clog.h"
#include "tools.h"
#include "CMTClient.h"
#include "CSpeedPosConfig.h"
#include "../Base/Comm/clib_mysql.h"
#include "../Base/Comm/comm_protocol.h"


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

	XMLNode xTTC;
	INT32 iUseL5 = 0;
    //id_gen_server
    fprintf( stderr, "load id_server...\n" );
	FETCH_SERVER_INFO("id_server", m_stIdServer);

    //trade_server
    fprintf( stderr, "load trade_server...\n" );
	FETCH_SERVER_INFO("trade_server", m_sttradeServer);

    //paygate_server
    fprintf( stderr, "load paygate_server...\n" );
	FETCH_SERVER_INFO("paygate_server", m_stpaygateServer);

    //auth_server
    fprintf( stderr, "load auth_server...\n" );
	FETCH_SERVER_INFO("auth_server", m_stAuthServer);

    //bill_server
    fprintf( stderr, "load bill_server...\n" );
	FETCH_SERVER_INFO("bill_server", m_stbillServer);

    //account_server
    fprintf( stderr, "load account_server...\n" );
	FETCH_SERVER_INFO("account_server", m_stAccountServer);

	fprintf(stderr, "load base config db...\n");
	XMLNode xDbNode = xMainNode.getChildNode("base_config_db");
	std::string shopIp = xDbNode.getAttribute("ip");
	int shop_port = atoi(xDbNode.getAttribute("port"));
	std::string shopUser = xDbNode.getAttribute("user");
	std::string shopPwd = xDbNode.getAttribute("pwd");
	m_stBaseDbName = xDbNode.getAttribute("db");

	pBaseDB = new clib_mysql;
	pBaseDB->init(shopIp.c_str(), shop_port, shopUser.c_str(), shopPwd.c_str());
	
	fprintf(stderr, "load business config  ..\n");
	XMLNode xPayCfgNode = xMainNode.getChildNode("busi_config");
	std::string strCfgPath = xPayCfgNode.getAttribute("confpath");

	//开始加载bill_busi_conf.xml 配置内容
	m_routeBillBusiConf = new CRouteBillBusiConf(strCfgPath.c_str());

	pDBPool = new CDBPool;
	std::vector<CRouteBillBusiConf::DbCfg>::iterator it_dbcfg = m_routeBillBusiConf->dbCfgVec.begin();
	for (; it_dbcfg != m_routeBillBusiConf->dbCfgVec.end(); ++it_dbcfg)
	{
		pDBPool->AddDBConn(it_dbcfg->db_num, it_dbcfg->type, it_dbcfg->host, it_dbcfg->port, it_dbcfg->user, it_dbcfg->pswd);
	}


    return 0;
}

void CSpeedPosConfig::PrintConfig()
{
//    fprintf( stderr, "------------- Agentpay Server, Build Time %s:%s------------------\n",
//            __DATE__, __TIME__ );

}

INT32 CSpeedPosConfig::InitLog()
{
    return c_log_init(m_szLogDir, m_iLogLevel, m_iLogSize, m_szLogPrefix);
}

INT32 CSpeedPosConfig::InitServer()
{
	m_idSocket = new CSocket;
	m_idSocket->SetServer( m_stIdServer );

	m_tradeSocket = new CSocket;
	m_tradeSocket->SetServer( m_sttradeServer );

	m_paygateSocket = new CSocket;
	m_paygateSocket->SetServer( m_stpaygateServer );

	m_authSocket = new CSocket;
	m_authSocket->SetServer( m_stAuthServer );

	m_billSocket = new CSocket;
	m_billSocket->SetServer( m_stbillServer );

	m_accountSocket = new CSocket;
	m_accountSocket->SetServer( m_stAccountServer );

    return 0;
}


