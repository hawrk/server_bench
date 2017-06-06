/*************************************************************************
    * File: COrderRefundDao.h
    * Brief: 订单退款流水TTC接口
    * Author: lining
    * Mail: lining55great@163.com
    * Created Time: Fri 02 Sep 2016 01:42:16 PM CST
 ************************************************************************/

#ifndef _C_PAY_TRANSACTION_FLOW_DAO_
#define _C_PAY_TRANSACTION_FLOW_DAO_

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

class clib_mysql;

class CPayTransactionFlowDao : public CObject
{
    public:
		CPayTransactionFlowDao();
		virtual ~CPayTransactionFlowDao();

        static const INT32 RET_HASREC = 1;
        static const INT32 RET_HASNOREC = 0;

        // 
		int GetOrderTableAllMchIdSql(clib_mysql& sql_instance,
								   const std::string& strTableName,
                                   const std::string& strBmId,
								   const int& iBeginTime,
								   const int& iEndTime,
                                   std::vector<std::string>& vecMchIds,
								   bool bFirstTab);

      
		int GetOrderTableFlowDataSql(clib_mysql& sql_instance,
										const std::string& strTableName,
										const std::string& strBmId,
										const string& strMchId,
										const int& iBeginTime,
										const int& iEndTime,
										std::vector<OrderFlowData>& vecOrderFlowDatas);

		int GetOrderChannelTableFlowDataSql(clib_mysql& sql_instance,
											const std::string& strTableName,
											const std::string& strOrderNo,
											std::vector<OrderChannelFlowData>& vecOrderChannelFlowDatas);

		int GetOrderRefundTableFlowDataSql(clib_mysql& sql_instance,
										const std::string& strBmId,
										const int& iStatus,
										const int& iBeginTime,
										const int& iEndTime,
										std::vector<OrderRefundFlowData>& vecOrderRefundFlowDatas);

		int GetOrderRefundChannelTableFlowDataSql(clib_mysql& sql_instance,
												const std::string& strRefundNo,
												std::vector<OrderRefundChannelFlowData>& vecOrderRefundChannelFlowDatas);

		/*
		 * @brief 清空对账表数据
		 */
		INT32 EmptyTableData(clib_mysql& sql_instance,
											const std::string& TableName, const std::string& strBmid,
											const std::string& strBeginTime,const std::string& strEndTime);

		INT32 InsertPayIdenticalWxToDB(clib_mysql& sql_instance, 
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);

		INT32 InsertRefundIdenticalWxToDB(clib_mysql& sql_instance, 
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);

		INT32 InsertPayDistinctWxToDB(clib_mysql& sql_instance, 
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);

		INT32 InsertRefundDistinctWxToDB(clib_mysql& sql_instance, 
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);

		int GetPayBillData(clib_mysql& sql_instance,
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
							std::map<std::string, OrderPayBillSumary>& orderPayBillSMap);

		int GetRefundBillData(clib_mysql& sql_instance,
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
							std::map<std::string, OrderRefundBillSumary>& orderRefundBillSMap);

		int GetChannelBillData(clib_mysql& sql_instance, const std::string& strTableFix, const std::string& strBmId,
						const std::string& strBeginTime, const std::string& strEndTime,
						const std::string& order_status, std::map<std::string, int>& channelMap);

		int GetWxOverFlowData(clib_mysql& sql_instance,
						const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
						std::vector<WxFlowSummary>& wxOverFlowVec);

		int GetAliOverFlowData(clib_mysql& sql_instance,
					const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
					std::vector<AliFlowSummary>& aliOverFlowList);


		INT32 InsertTradeTypeOrderToDB(clib_mysql& sql_instance, const std::string& strBmId,
								const std::string& pay_channel, const std::string& strTableName);

		INT32 InsertTradeTypeOrderChannelToDB(clib_mysql& sql_instance, const std::string& strBmId,
								const std::string& pay_channel, const std::string& strTableName);

		INT32 TruncateEveryPaymentTypeSysFlowData(clib_mysql& sql_instance, const std::string& strBmId);

		INT32 RemoveAlipayDifferenceSuccState(clib_mysql& sql_instance, const std::string& strBmId);

		INT32 RemoveAlipayDifferenceRefundState(clib_mysql& sql_instance, const std::string& strBmId);

		INT32 InsertAliPayIdenticalToDB(clib_mysql& sql_instance,
					const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime, const std::string& order_status);

		INT32 InsertAliPayIdenticalRefundToDB(clib_mysql& sql_instance,
					const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);
	
		INT32 InsertAliPayDistinctToDB(clib_mysql& sql_instance,
					const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime, const std::string& order_status);

		INT32 InsertAliPayDistinctRefundToDB(clib_mysql& sql_instance,
					const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);
};
#endif  /*_CORDER_REFUND_DAO_H_*/
