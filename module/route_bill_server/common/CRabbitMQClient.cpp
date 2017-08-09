/*
 * CRabbitMQClient.cpp
 *
 *  Created on: 2012-12-12
 *      Author: sangechen
 */

#include "CRabbitMQClient.h"
#include "log/clog.h"

CRabbitMQClient::CRabbitMQClient()
{
	 
}

CRabbitMQClient::~CRabbitMQClient()
{
    finiContext();
}

int CRabbitMQClient::Init(const char* szCfg, const char* szExchangName, const char* szQueueName, const char* szQueueKey)
{
	CDEBUG_LOG("init begin \n");
	m_szCfg = szCfg;
	m_szExchangName = szExchangName;
	m_szQueueName = szQueueName;
	m_szQueueKey = szQueueKey;	
	return (initContext()) ? 0 : -10;;
}


bool CRabbitMQClient::initContext()
{
	try
	{
		pAmQp = new AMQP(m_szCfg);
		if (NULL == pAmQp)
		{	return false;	}
		
		pExchange = pAmQp->createExchange(m_szExchangName);
		if (NULL == pExchange)
		{	return false;	}
		
		pExchange->Declare(m_szExchangName, "direct", 2);
		
		pQueue = pAmQp->createQueue(m_szQueueName);
		if (NULL == pQueue)
		{	return false;	}
		
		pQueue->Declare(m_szQueueName, 2);
		pQueue->Bind(m_szExchangName, m_szQueueKey);
		CDEBUG_LOG("init rabbitmq connect succeed\n");
	}
	catch (AMQPException& e)
	{
		CERROR_LOG("init rabbitmq connection error, msg:%s\n", e.getMessage().c_str());
		finiContext();

        return false;
	}
	catch(...)
	{
		CERROR_LOG("init rabbitmq connection error!\n");
		finiContext();

        return false;		
	}

    return true;
}

int CRabbitMQClient::Push(const std::string data, const std::string& key)
{
	int i = 0;
	for (i = 0; i < 5; ++i)//重连5次
	{
		try
		{
			CDEBUG_LOG("Push Data [%s] key [%s]\n", data.c_str(), key.c_str());
			pExchange->setHeader("Delivery-mode", 2);
			pExchange->setHeader("Content-type", "text/text");
			pExchange->setHeader("Content-encoding", "UTF-8");
			pExchange->Publish(data, key);
			CDEBUG_LOG("Push Data success!");
			break;
		}
		catch (AMQPException& e)
		{
			//Push MQ 失败 
			CERROR_LOG("rabbitmq push error, msg:%s\n", e.getMessage().c_str());
			finiContext();
			initContext();
		}
		catch(...)
		{
			//Push MQ 失败 
			CERROR_LOG("rabbitmq push error!");
			finiContext();
			initContext();
		}
	}

	if(i==5)
	{	return -1;	}
	else
	{	return 0;	}
}


void CRabbitMQClient::finiContext()
{
   if (pAmQp)
   {
	   delete pAmQp;
	   pAmQp = NULL;
	   pExchange = NULL;
	   pQueue = NULL;
   }
  
}



/*
int main(int argc, char **argv) {
    CRabbitMQClient redis_cli;

    redis_cli.Init("cps_order_sync", "10.130.133.150", 13487);

    redis_cli.RPush("test sdfasdfads sdfasdf ", 5);
    redis_cli.RPush("test sdfasdfads sdfasdf ", 6);
    redis_cli.RPush("test sdfasdfads sdfasdf ", 12);
    redis_cli.RPush("test sdfasdfads sdfasdf ", 1);
}
*/

