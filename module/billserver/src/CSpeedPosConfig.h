/*
 * COrderConfig.h
 *
 *  Created on: 2010-5-27
 *      Author: rogeryang
 */

#ifndef _CSPEEDPOS_CONFIG_H_
#define _CSPEEDPOS_CONFIG_H_


//#define _SYBASE

#include "CObject.h"
#include "CSpeedPosServer.h"
#ifdef _SYBASE
#include "CSybase.h"
#include "CPayTransSybaseDao.h"
#else
#include "CPayTransactionFlowDao.h"
#include "../../Base/Comm/clib_mysql.h"
#include "DBPool.h"
#endif
#include "dc/dcreporter.h"
#include "CSpeedPosServer.h"
#include "CBillBusiConfig.h"

using base::comm::DCReporter;

class CSpeedPosConfig : public CObject
{
    public:
		CSpeedPosConfig(){	
#ifdef _SYBASE
#else
			pShopDB = NULL;
			pBillDB = NULL;
#endif
			m_SingleLimitAtLeastUnderlyingRule = false;
			iDbIndex = 0;
			pBusConfig = NULL;
		}
		virtual ~CSpeedPosConfig(){
#ifdef _SYBASE

#else
			if (pShopDB)
			{
				delete pShopDB;
				pShopDB = NULL;
			}

			if (pDBPool)
			{
				delete pDBPool;
				pDBPool = NULL;
			}
			if (pBillDB)
			{ delete pBillDB; pBillDB = NULL; }
#endif
		}

        INT32 LoadConfig( const CHAR* pchConfig );
        void PrintConfig();

        INT32 InitLog();
		INT32 InitOrderServer(CSpeedPosServer& order);

        INT32 InitDCReporter(DCReporter& reporter) {
            return reporter.Init(reporter_id_);
        }
#ifdef _SYBASE

        CSyBase* GetBillDB()
		{
			return pBillDB;
		}

#else
		clib_mysql* GetShopDB()
		{
			return pShopDB;
		}

		clib_mysql* GetBillDB()
		{
			return pBillDB;
		}

		CDBPool* GetDBPool()
		{
			return pDBPool;
		}


		clib_mysql* GetJumpOrderDB(const std::string& order_no)
		{
			int iDbNum = atoi(order_no.substr(order_no.length() - 2, 2).c_str());
			return pDBPool->GetDBConn(iDbNum, 1);
		}
#endif
		
		const string& getShopDbName()
		{
			return m_stShopDbName;
		}

		const std::string GetBillDbName()
		{
			return m_stBillDbName;
		}

		CBillBusiConfig* GetBillBusiConfig()
		{
			return pBusConfig;
		}


        key_t GetNotifyQueueKey()
        {
            return m_kNotifyKey;
        }

        const string& getSecureToken()
        {
            return m_sSecureToken;
        }

		const string& getMerchantPrefix()
		{
			return m_strMerPrefix;
		}

		const INT32 getMerchantIdLen()
		{
			return m_iMerIdLen;
		}

        const char* getShopLockFile() const
        {
			return m_strMerchantIdLockFile.c_str();
        };

        const char* getUinLockFileFilterSvr() const
        {
            return m_strUinLockFileFilterSvr.c_str();
        };

        bool IsSyncCreatePayLog() {
            return is_sync_create_pay_log_;
        }

private:
#ifdef _SYBASE
        CSyBase* pBillDB;

#else
		clib_mysql* pShopDB;

		clib_mysql* pBillDB;

		CDBPool* pDBPool;
#endif

		string m_stShopDbName;

		string m_stBillDbName;

		CBillBusiConfig* pBusConfig;

		int iDbIndex;
        //base
        CHAR m_szLogDir[1024];
        CHAR m_szLogPrefix[1024];
        INT32 m_iLogSize;
        INT32 m_iLogLevel;
        string m_sTokenPrefix;//默认token前缀
        UINT32 m_uiTokenLen;//默认token长度，前缀为TOKEN_PREFIX
        string m_sSerialPrefix;//默认serial前缀
        UINT32 m_uiSerialLen;//默认serial长度，前缀为SERIAL_PREFIX
        //UINT32 m_uiExpireTime;//过期时间阀值,放到SPinfo表中

        string m_strGrouponLockFile;
		string m_strMerchantIdLockFile;
		string m_strUinLockFileFilterSvr;

		std::string m_strExchangName;
		std::string m_strQueueName;
		std::string m_strQueueKey;
        //extern interface
       

		//tradeServer
        CommServer m_stTradeServer;
        //serttle server
        CommServer m_stSettleServer;
        //bill server 在这里就是自已调用自已
        CommServer m_stBillServer;

		
        //notify
        key_t m_kNotifyKey;

        //merchartid
		string m_strMerPrefix;
		INT32 m_iMerIdLen;

        //token
        string m_sSecureToken;


        bool   m_SingleLimitAtLeastUnderlyingRule;

        //Redis
        CommServer m_stRedisServer;
        string m_sListKeyName;
		string m_sListChnelKeyName;

        //dc reporter
        string reporter_id_;

        // 是否同步订单日志
        bool is_sync_create_pay_log_;
};

#endif /* CORDERCONFIG_H_ */

