/*
 * COrderConfig.h
 *
 *  Created on: 2010-5-27
 *      Author: rogeryang
 */

#ifndef _CSPEEDPOS_CONFIG_H_
#define _CSPEEDPOS_CONFIG_H_
#include "CObject.h"
#include "../Base/Comm/clib_mysql.h"
#include "CRouteBillBusiConf.h"
#include "DBPool.h"
#include "dc/dcreporter.h"
#include "CSocket.h"

using base::comm::DCReporter;

class CSpeedPosConfig : public CObject
{
    public:
		CSpeedPosConfig(){	
			pBaseDB = NULL;
			pDBPool = NULL;
			iDbIndex = 0;
			m_iLogSize = 0;
			m_iLogLevel = 0;

			m_idSocket = NULL;
			m_tradeSocket = NULL;
			m_paygateSocket = NULL;
			m_authSocket = NULL;
			m_billSocket = NULL;
			m_routeBillBusiConf = NULL;
		}
		virtual ~CSpeedPosConfig(){
			if (pBaseDB)
			{
				delete pBaseDB;
				pBaseDB = NULL;
			}

			if (pDBPool)
			{
				delete pDBPool;
				pDBPool = NULL;
			}


			if(m_idSocket)
			{
				delete m_idSocket;
				m_idSocket = NULL;
			}

			if(m_tradeSocket)
			{
				delete m_tradeSocket;
				m_tradeSocket = NULL;
			}

			if(m_paygateSocket)
			{
				delete m_paygateSocket;
				m_paygateSocket = NULL;
			}

			if(m_authSocket)
			{
				delete m_authSocket;
				m_authSocket = NULL;
			}

			if(m_billSocket)
			{
				delete m_billSocket;
				m_billSocket = NULL;
			}


			if(m_routeBillBusiConf)
			{
				delete m_routeBillBusiConf;
				m_routeBillBusiConf = NULL;
			}
		}

        INT32 LoadConfig( const CHAR* pchConfig );
        void PrintConfig();

        INT32 InitLog();
		INT32 InitServer();

		clib_mysql* GetBaseDB()
		{
			return pBaseDB;
		}

		CDBPool* GetDBPool()
		{
			return pDBPool;
		}


		const string& getBaseDbName()
		{
			return m_stBaseDbName;
		}

		CRouteBillBusiConf* GetBusiConf()
		{
			return m_routeBillBusiConf;
		}

		CSocket* GetIdServerSocket()
		{
			return m_idSocket;
		}

		CSocket* GetTradeServerSocket()
		{
			return m_tradeSocket;
		}

		CSocket* GetPaygateServerSocket()
		{
			return m_paygateSocket;
		}

		CSocket* GetAuthServerSocket()
		{
			return m_authSocket;
		}

		CSocket* GetBillServerSocket()
		{
			return m_billSocket;
		}


private:

		clib_mysql* pBaseDB;

		CDBPool* pDBPool;


		int iDbIndex;

        //base
        CHAR m_szLogDir[1024];
        CHAR m_szLogPrefix[1024];
        INT32 m_iLogSize;
        INT32 m_iLogLevel;

		CSocket* m_idSocket;
        CommServer m_stIdServer;

		CSocket* m_tradeSocket;
        CommServer m_sttradeServer;

		CSocket* m_paygateSocket;
        CommServer m_stpaygateServer;

        CSocket* m_authSocket;
		CommServer m_stAuthServer;

        CSocket* m_billSocket;
		CommServer m_stbillServer;

        	
		string m_stBaseDbName;

		CRouteBillBusiConf* m_routeBillBusiConf;
};

#endif /* CORDERCONFIG_H_ */

