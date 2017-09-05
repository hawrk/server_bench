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
#include "mysqlapi.h"
#include "speed_bill_protocol.h"
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
		//本地和渠道都是成功
		INT32 InsertPayIdenticalWxToDB(clib_mysql& sql_instance, 
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);

		//本地和渠道都是退款
		INT32 InsertRefundIdenticalWxToDB(clib_mysql& sql_instance, 
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);

		//金额不符
		INT32 InsertAmountNotMatchToDB(clib_mysql& sql_instance,
							const std::string& strBmId,const std::string& strPayChannel,
							const std::string& strBillDate,const std::string& strBatchNo,
							const std::string& strBeginTime, const std::string& strEndTime);

		//微信成功多
		INT32 InsertPayDistinctWxToDB(clib_mysql& sql_instance, 
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);
		//微信退款的多
		INT32 InsertRefundDistinctWxToDB(clib_mysql& sql_instance, 
							const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime);

		//本地成功多
		INT32 InsertPaySuccessToDB(clib_mysql& sql_instance, const std::string& strBmId,
				const std::string& strBeginTime, const std::string& strEndTime,const std::string& strPayChannel);
		//本地退款多
		INT32 InsertPayRefundToDB(clib_mysql& sql_instance, const std::string& strBmId,
				const std::string& strBeginTime, const std::string& strEndTime,const std::string& paychannel);

		//生成本地对账成功的对账单
		int GetCheckedBillData(clib_mysql& sql_instance,const std::string& strBmId, const std::string strChannel,
							const std::string& strBeginTime, const std::string& strEndTime,std::vector<CheckedBillData>& vecbilldata);
		//

		int GetPayBillData(clib_mysql& sql_instance,
							const std::string& strBmId, const std::string strChannel, const std::string& strBeginTime, const std::string& strEndTime,
							std::map<std::string, OrderPayBillSumary>& orderPayBillSMap);

		int GetRefundBillData(clib_mysql& sql_instance,
							const std::string& strBmId, const std::string strChannel,const std::string& strBeginTime, const std::string& strEndTime,
							std::map<std::string, OrderRefundBillSumary>& orderRefundBillSMap);

		int GetChannelBillData(clib_mysql& sql_instance, const std::string& strTableFix, const std::string& strBmId,const std::string& Chann,
						const std::string& strBeginTime, const std::string& strEndTime,
						const std::string& order_status, std::map<std::string, OrderChannelFlowData>& channelMap);

		int GetWxOverFlowData(clib_mysql& sql_instance,
						const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
						std::vector<WxFlowSummary>& wxOverFlowVec);

		/*
		 * 状态不一致，补单 BEGIN
		 */
		INT32 InsertToChannelFlow(clib_mysql& sql_instance,const string& tableName,StringMap& channelMap);

		INT32 InsertTOOrderSuccFlow(clib_mysql& sql_instance,StringMap& orderMap);

		INT32 UpdateAbnormalStatus(clib_mysql& sql_instance,StringMap& order_no);

		/*
		 * 补单操作 END
		 */


		//清分表操作
		INT32 InsertDistributionDB(clib_mysql& sql_instance,const std::string& strBmId,const std::string& bill_date,const std::string& batch_no,
						const std::string& pay_channel,const std::string& channel_code,OrderStat& ordStat,TRemitBill& remitBill,const char* fund_type);

		//结算表操作
		INT32 InsertSettleDB(clib_mysql& sql_instance,const std::string& strBmId,const std::string& bill_date,const std::string& batch_no,
						const std::string& pay_channel,TRemitBill& remitBill);

		INT32 InsertwxAbnormalDB(clib_mysql& sql_instance,const std::string& strBmId,const std::string& bill_date,const std::string& batch_no,
						const std::string& pay_channel,std::vector<WxFlowSummary>& wxOverFlowVec,int index);

		INT32 InsertaliAbnormalDB(clib_mysql& sql_instance,const std::string& strBmId,const std::string& bill_date,const std::string& batch_no,
						const std::string& pay_channel,std::vector<AliFlowSummary>& aliOverFlowVec,int index);

		int GetAliOverFlowData(clib_mysql& sql_instance,
					const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
					std::vector<AliFlowSummary>& aliOverFlowList);

		INT32 InsertSummaryDB(clib_mysql& sql_instance,const std::string& strBmId,const std::string& bill_date,const std::string& batch_no,
				const std::string& strBeginTime,const std::string& strEndTime,const char* pay_channel);


		INT32 InsertTradeTypeOrderToDB(clib_mysql& sql_instance, const std::string& strBmId,
								const std::string& pay_channel, const std::string& strTableName);

		INT32 InsertTradeTypeOrderChannelToDB(clib_mysql& sql_instance, const std::string& strBmId,
								const std::string& pay_channel, const std::string& strTableName);

		INT32 TruncateEveryPaymentTypeSysFlowData(clib_mysql& sql_instance, const std::string& strBmId,const std::string& pay_channel);

		INT32 LoadFiletoDB(clib_mysql& sql_instance, const std::string& strFileName,const std::string& strTableName);

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
