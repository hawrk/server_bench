/*
 * CRedisClient.h
 *
 *  Created on: 2012-12-12
 *      Author: sangechen
 */

#ifndef _C_RABBIT_MQ_CLIENT_H_
#define _C_RABBIT_MQ_CLIENT_H_

#include <string.h>
#include "AMQPcpp.h"

#include "Base/Comm/ISynchronized.h"

class CRabbitMQClient : public ISynchronized
{
public:
	CRabbitMQClient();
	virtual ~CRabbitMQClient();

	int Init(const char* szCfg, const char* szExchangName, const char* szQueueName, const char* szQueueKey);

	int Push(const std::string data, const std::string& key);

protected:

	bool initContext();
	
	void finiContext();

private:
	AMQP* pAmQp;

	AMQPExchange* pExchange;

	AMQPQueue* pQueue;

   std::string m_szCfg;

   std::string m_szExchangName;

   std::string m_szQueueName;

   std::string m_szQueueKey;

};

#endif /* REDISCLIENT_H_ */
