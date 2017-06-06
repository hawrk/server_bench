/*
 * CPayTransSybaseDao.h
 *
 *  Created on: 2017年5月27日
 *      Author: hawrkchen
 */

#ifndef _CPAYTRANSSYBASEDAO_H_
#define _CPAYTRANSSYBASEDAO_H_

#include <vector>
#include <stdint.h>
#include <string>
#include <vector>
#include "CObject.h"
//#include "comm_protocol.h"
#include "types.h"
#include <map>
#include <list>
#include "../business/bill_protocol.h"
#include "common.h"
#include "CSybase.h"
#include "spp_query_1.h"


class CSyBase;

class CPayTransSybaseDao : public CObject
{
    public:
		CPayTransSybaseDao();
		virtual ~CPayTransSybaseDao();

        static const INT32 RET_HASREC = 1;
        static const INT32 RET_HASNOREC = 0;

		/*
		 * @brief 清空对账表数据
		 */
		//use
		INT32 EmptyTableData(CSyBase& sql_instance,
											const std::string& TableName, const std::string& strBmid,
											const std::string& strBeginTime,const std::string& strEndTime);
		//use
		INT32 InsertPayIdenticalWxToDB(CSyBase& sql_instance,
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);
		//use
		INT32 InsertRefundIdenticalWxToDB(CSyBase& sql_instance,
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);
		//use
		INT32 InsertPayDistinctWxToDB(CSyBase& sql_instance,
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);
		//use
		INT32 InsertRefundDistinctWxToDB(CSyBase& sql_instance,
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);

		//use
		int GetPayBillData(CSyBase& sql_instance,
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
							std::map<std::string, OrderPayBillSumary>& orderPayBillSMap);
		//use
		int GetRefundBillData(CSyBase& sql_instance,
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
							std::map<std::string, OrderRefundBillSumary>& orderRefundBillSMap);
		//use
		int GetChannelBillData(CSyBase& sql_instance, const std::string& strTableFix, const std::string& strBmId,
						const std::string& strBeginTime, const std::string& strEndTime,
						const std::string& order_status, std::map<std::string, int>& channelMap);
		//use
		INT32 GetWxOverFlowData(CSyBase& sql_instance,
						const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
						std::vector<WxFlowSummary>& wxOverFlowVec);
		//use
		INT32 GetAliOverFlowData(CSyBase& sql_instance,
					const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
					std::vector<AliFlowSummary>& aliOverFlowList);

		//use
		INT32 InsertTradeTypeOrderToDB(CSyBase& sql_instance, const std::string& strBmId,
								const std::string& pay_channel, const std::string& strTableName);
		//use
		INT32 InsertTradeTypeOrderChannelToDB(CSyBase& sql_instance, const std::string& strBmId,
								const std::string& pay_channel, const std::string& strTableName);

		INT32 TruncateEveryPaymentTypeSysFlowData(CSyBase& sql_instance, const std::string& strBmId,const string& channel);

		//use
		INT32 RemoveAlipayDifferenceSuccState(CSyBase& sql_instance, const std::string& strBmId);
		//use
		INT32 RemoveAlipayDifferenceRefundState(CSyBase& sql_instance, const std::string& strBmId);
		//use
		INT32 InsertAliPayIdenticalToDB(CSyBase& sql_instance,
					const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime, const std::string& order_status);
		//use
		INT32 InsertAliPayIdenticalRefundToDB(CSyBase& sql_instance,
					const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);
		//use
		INT32 InsertAliPayDistinctToDB(CSyBase& sql_instance,
					const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime, const std::string& order_status);
		//use
		INT32 InsertAliPayDistinctRefundToDB(CSyBase& sql_instance,
					const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);

};



#endif /* _CPAYTRANSSYBASEDAO_H_ */
