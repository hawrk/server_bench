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
#include "CBaseBusiConf.h"
#include "DBPool.h"
#include "dc/dcreporter.h"
#include "CSocket.h"

using base::comm::DCReporter;

class CSpeedPosConfig : public CObject
{
    public:
		CSpeedPosConfig(){	
			pBaseDB = NULL;
			iDbIndex = 0;
			m_iLogSize = 0;
			m_iLogLevel = 0;

			m_idSocket = NULL;
			m_baseBusiConf = NULL;
		}
		virtual ~CSpeedPosConfig(){
			if (pBaseDB)
			{
				delete pBaseDB;
				pBaseDB = NULL;
			}


			if(m_idSocket)
			{
				delete m_idSocket;
				m_idSocket = NULL;
			}


			if(m_baseBusiConf)
			{
				delete m_baseBusiConf;
				m_baseBusiConf = NULL;
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


		const string& getBaseDbName()
		{
			return m_stBaseDbName;
		}

		CBaseBusiConf* GetBusiConf()
		{
			return m_baseBusiConf;
		}

		CSocket* GetIdServerSocket()
		{
			return m_idSocket;
		}


private:

		clib_mysql* pBaseDB;


		int iDbIndex;

        //base
        CHAR m_szLogDir[1024];
        CHAR m_szLogPrefix[1024];
        INT32 m_iLogSize;
        INT32 m_iLogLevel;

		CSocket* m_idSocket;
        CommServer m_stIdServer;
        	
		string m_stBaseDbName;

		CBaseBusiConf* m_baseBusiConf;
};

#endif /* CORDERCONFIG_H_ */

