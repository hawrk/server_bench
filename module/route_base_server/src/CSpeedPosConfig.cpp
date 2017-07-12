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
#include "comm_protocol.h"


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

	//settle_gen_server
	//FETCH_SERVER_INFO("settle_server",m_stSettleServer);

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
	m_baseBusiConf = new CBaseBusiConf(strCfgPath.c_str());


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

    return 0;
}


