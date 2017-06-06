/*
 * CIdGenClient.h
 *
 *  Created on: 2010-5-20
 *      Author: rogeryang
 */

#ifndef C_SPP_CLIENT_H_
#define C_SPP_CLIENT_H_
#include <string>
#include <vector>
#include "CObject.h"
#include "comm_protocol.h"
#include "CSocket.h"
#include "../business/bill_protocol.h"
#include "cJSON.h"
#include "json_util.h"

using namespace std;

class CSppClient : public CObject
{
public:
	CSppClient();
    virtual
		~CSppClient();

    void Reset()
    {
        CObject::Reset();
        m_cSocket.Reset();
    }

    INT32 Init( const CommServer& stServer);
    

	INT32 CallOrderQuery(const std::string& strOrderNo, SOrderInfoRsp& rsp);

	//call settle server
	//发送对账结果通知给结算服务
	INT32 SendBillNotify( StringMap&parammap, SOrderInfoRsp& rsp);


protected:
    CSocket m_cSocket;
};

#endif /* CIDGENCLIENT_H_ */
