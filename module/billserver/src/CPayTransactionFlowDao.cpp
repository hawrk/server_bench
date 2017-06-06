/*************************************************************************
    * File: CPayTransactionFlowDao.cpp
    * Brief: 订单退款TTC逻辑
    * Author: lining
    * Mail: lining55great@163.com
    * Created Time: Fri 02 Sep 2016 02:12:35 PM CST
 ************************************************************************/

#include "CPayTransactionFlowDao.h"
#include <inttypes.h>
#include <sys/time.h>
#include <sys/types.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "log/clog.h"
#include "clib_mysql.h"
#include "../../Base/include/mysql/mysql.h"
#include "tools.h"
#include "l5_wrap.h"
#include "../../Base/Comm/CObject.h"
#include "../../Base/Comm/comm_protocol.h"
#include "../../Base/Comm/groupon_common.h"
#include "../../Base/Comm/types.h"
#include "tools.h"

CPayTransactionFlowDao::CPayTransactionFlowDao()
{
    // TODO;
}

CPayTransactionFlowDao::~CPayTransactionFlowDao()
{
    // TODO;
}


/**
 * brief: 
 * param: 
 * out: 
 * return:
 * */
INT32 CPayTransactionFlowDao::GetOrderTableAllMchIdSql(clib_mysql& sql_instance, 
													   const std::string& strTableName, 
													   const std::string& strBmId,
													   const int& iBeginTime, 
													   const int& iEndTime, 
													   std::vector<std::string>& vecMchIds,
													   bool bFirstTab)
{
    BEGIN_LOG( __func__ );
    int32_t iRet = 0;
    MYSQL_ROW row;

    Reset();

    char sql_stmt[4096]; 

	snprintf(sql_stmt, sizeof(sql_stmt),
		" SELECT "
		" DISTINCT mch_id "
		" FROM %s "
		" WHERE "
		" bm_id = %s "
		" AND pay_time >= %d "
		" AND pay_time <= %d "
		" AND order_status IN (2, 4, 5)",
		strTableName.c_str(), strBmId.c_str(), iBeginTime, iEndTime);

	if(bFirstTab)
	{
		CDEBUG_LOG( "GetOrderTableAllMchIdSql sql: %s", sql_stmt);
	}
    
    iRet = sql_instance.query(sql_stmt);

    //CDEBUG_LOG( "GetOrderTableAllMchIdSql query end!" );
    if (iRet != 0)
    {
        snprintf(m_szErrMsg, sizeof(m_szErrMsg),
                 "GetOrderTableAllMchIdSql Execute Failed.Ret[%d] Err[%u~%s]",
                 iRet, sql_instance.get_errno(), sql_instance.get_error());
        return -20;
    }

    if (sql_instance.num_rows() <= 0)
    {
        // 数据不存在
        return RET_HASNOREC;
    }    

    CDEBUG_LOG( "GetOrderTableAllMchIdSql set begin!" );
	for (int i = 0; i < sql_instance.num_rows(); i++)
	{
		if ((row = sql_instance.fetch_row()))
		{
			std::string strMchId = row[0];
			vecMchIds.push_back(strMchId);
		}
		else
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg),
				"FetchRow Failed.Ret[%d] Err[%u~%s]",
				iRet, sql_instance.get_errno(), sql_instance.get_error());
			return -30;
		}
	}
	sql_instance.free_result();

	return RET_HASREC;
}

int CPayTransactionFlowDao::GetOrderTableFlowDataSql(clib_mysql& sql_instance, 
													const std::string& strTableName, 
													const std::string& strBmId,
													const string& strMchId,
													const int& iBeginTime, 
													const int& iEndTime, 
													std::vector<OrderFlowData>& vecOrderFlowDatas)
{
	BEGIN_LOG(__func__);
	int iRet = 0;
	MYSQL_ROW row;

	Reset();
	vecOrderFlowDatas.clear();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" SELECT "
		" mch_id, channel_id, order_no, out_trade_no,"
		" pay_channel, transaction_id, trade_type, total_fee,"
		" refund_fee, order_status, shop_amount, payment_profit,"
		" channel_profit, service_profit, bm_profit, total_commission, pay_time"
		" FROM %s "
		" WHERE "
		" bm_id = %s "
		" AND pay_time >= %d "
		" AND pay_time <= %d "
		" AND mch_id=%s "
		" AND order_status IN (2, 4, 5) ORDER BY pay_time ",
		strTableName.c_str(), strBmId.c_str(), iBeginTime, iEndTime, strMchId.c_str());

	CDEBUG_LOG("GetOrderTableFlowDataSql sql: %s", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetOrderTableFlowDataSql Execute Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -20;
	}

	if (sql_instance.num_rows() <= 0)
	{
		// 数据不存在
		return RET_HASNOREC;
	}

	for (int i = 0; i < sql_instance.num_rows(); ++i)
	{
		OrderFlowData orderFlow;
		orderFlow.Reset();
		if ((row = sql_instance.fetch_row()))
		{
			orderFlow.mch_id = (row[0]) ? row[0] : "";
			orderFlow.channel_id = (row[1]) ? row[1] : "";
			orderFlow.order_no = (row[2]) ? row[2] : "";
			orderFlow.out_trade_no = (row[3]) ? row[3] : "";
			orderFlow.pay_channel = (row[4]) ? row[4] : "";
			orderFlow.transaction_id = (row[5]) ? row[5] : "";
			orderFlow.trade_type = (row[6]) ? row[6] : "";
			orderFlow.total_fee = (row[7]) ? atoll(row[7]) : 0;
			orderFlow.refund_fee = (row[8]) ? atoll(row[8]) : 0;
			orderFlow.order_status = (row[9]) ? atoll(row[9]) : 0;
			orderFlow.shop_amount = (row[10]) ? atoll(row[10]) : 0;
			orderFlow.payment_profit = (row[11]) ? atoll(row[11]) : 0;
			orderFlow.channel_profit = (row[12]) ? atoll(row[12]) : 0;
			orderFlow.service_profit = (row[13]) ? atoll(row[13]) : 0;
			orderFlow.bm_profit = (row[14]) ? atoll(row[14]) : 0;
			orderFlow.total_commission = (row[15]) ? atoll(row[15]) : 0;
			orderFlow.pay_time = (row[16]) ? atoll(row[16]) : 0;

			vecOrderFlowDatas.push_back(orderFlow);
		}
		else
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg),
				"FetchRow Failed.Ret[%d] Err[%u~%s]",
				iRet, sql_instance.get_errno(), sql_instance.get_error());
			return -30;
		}
	}
	sql_instance.free_result();

	return RET_HASREC;
}

int CPayTransactionFlowDao::GetOrderChannelTableFlowDataSql(clib_mysql& sql_instance,
															const std::string& strTableName,
															const std::string& strOrderNo, 
															std::vector<OrderChannelFlowData>& vecOrderChannelFlowDatas)
{
	BEGIN_LOG(__func__);
	int iRet = 0;
	MYSQL_ROW row;

	Reset();
	vecOrderChannelFlowDatas.clear();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" SELECT "
		" mch_id, order_no, channelid, total_fee,"
		" channel_profit_rate, channel_profit, paytime "
		" FROM %s "
		" WHERE "
		" order_no = '%s'",
		strTableName.c_str(), strOrderNo.c_str());

	CDEBUG_LOG("GetOrderChannelTableFlowDataSql sql: %s", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetOrderChannelTableFlowDataSql Execute Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -20;
	}

	if (sql_instance.num_rows() <= 0)
	{
		// 数据不存在
		return RET_HASNOREC;
	}

	for (int i = 0; i < sql_instance.num_rows(); ++i)
	{
		OrderChannelFlowData orderChannelFlow;
		orderChannelFlow.Reset();
		if ((row = sql_instance.fetch_row()))
		{
			orderChannelFlow.mch_id = (row[0]) ?row[0] : "";
			orderChannelFlow.order_no = (row[1]) ? row[1] : "";
			orderChannelFlow.channel_id = (row[2]) ? row[2] : "";
			orderChannelFlow.total_fee = (row[3]) ? atoll(row[3]) : 0;
			orderChannelFlow.channel_profit_rate = (row[4]) ? atoll(row[4]) : 0;
			orderChannelFlow.channel_profit = (row[5]) ? atoll(row[5]) : 0;
			orderChannelFlow.pay_time = (row[6]) ? atoll(row[6]) : 0;

			vecOrderChannelFlowDatas.push_back(orderChannelFlow);
		}
		else
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg),
				"FetchRow Failed.Ret[%d] Err[%u~%s]",
				iRet, sql_instance.get_errno(), sql_instance.get_error());
			return -30;
		}
	}
	sql_instance.free_result();

	return RET_HASREC;
}


int CPayTransactionFlowDao::GetOrderRefundTableFlowDataSql(clib_mysql& sql_instance,
														const std::string& strBmId,
														const int& iStatus,
														const int& iBeginTime, 
														const int& iEndTime, 
														std::vector<OrderRefundFlowData>& vecOrderRefundFlowDatas)
{
	BEGIN_LOG(__func__);
	int iRet = 0;
	MYSQL_ROW row;

	Reset();
	vecOrderRefundFlowDatas.clear();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" SELECT "
		" mch_id, channelid, pay_channel, trade_type,"
		" order_no, refund_no, out_refund_no, refund_id,"
		" refund_fee, total_fee, refund_payment_profit, refund_channel_profit,"
		" refund_service_profit, refund_bm_profit, refund_shop_amount, refund_total_commission,"
		" status, refundtime"
		" FROM shop_db.orders_refund "
		" WHERE "
		" bm_id = %s "
		" AND refundtime >= %d "
		" AND refundtime <= %d "
		" AND status = %d ORDER BY refundtime ",
		 strBmId.c_str(), iBeginTime, iEndTime, iStatus);

	CDEBUG_LOG("GetOrderRefundTableFlowDataSql sql: %s", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetOrderRefundTableFlowDataSql Execute Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -20;
	}

	if (sql_instance.num_rows() <= 0)
	{
		// 数据不存在
		return RET_HASNOREC;
	}

	for (int i = 0; i < sql_instance.num_rows(); ++i)
	{
		OrderRefundFlowData orderRefundFlow;
		orderRefundFlow.Reset();
		if ((row = sql_instance.fetch_row()))
		{
			orderRefundFlow.mch_id = (row[0]) ? row[0] : "";
			orderRefundFlow.channel_id = (row[1]) ?row[1] : "";
			orderRefundFlow.pay_channel = (row[2]) ? row[2] : "";
			orderRefundFlow.trade_type = (row[3]) ? row[3] : "";
			orderRefundFlow.order_no = (row[4]) ? row[4] : "";
			orderRefundFlow.refund_no = (row[5]) ? row[5] : "";
			orderRefundFlow.out_refund_no = (row[6]) ? row[6] : "";
			orderRefundFlow.refund_id = (row[7]) ? row[7] : "";
			orderRefundFlow.refund_fee = (row[8]) ? atoll(row[8]) : 0;
			orderRefundFlow.total_fee = (row[9]) ? atoll(row[9]) : 0;
			orderRefundFlow.refund_payment_profit = (row[10]) ? atoll(row[10]) : 0;
			orderRefundFlow.refund_channel_profit = (row[11]) ? atoll(row[11]) : 0;
			orderRefundFlow.refund_service_profit = (row[12]) ? atoll(row[12]) : 0;
			orderRefundFlow.refund_bm_profit = (row[13]) ? atoll(row[13]) : 0;
			orderRefundFlow.refund_shop_amount = (row[14]) ? atoll(row[14]) : 0;
			orderRefundFlow.refund_total_commission = (row[15]) ? atoll(row[15]) : 0;
			orderRefundFlow.status = (row[16]) ? atoll(row[16]) : 0;
			orderRefundFlow.refund_time = (row[17]) ? atoll(row[17]) : 0;

			vecOrderRefundFlowDatas.push_back(orderRefundFlow);
		}
		else
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg),
				"FetchRow Failed.Ret[%d] Err[%u~%s]",
				iRet, sql_instance.get_errno(), sql_instance.get_error());
			return -30;
		}
	}
	sql_instance.free_result();

	return RET_HASREC;
}



int CPayTransactionFlowDao::GetOrderRefundChannelTableFlowDataSql(clib_mysql& sql_instance,
																const std::string& strRefundNo,
																std::vector<OrderRefundChannelFlowData>& vecOrderRefundChannelFlowDatas)
{
	BEGIN_LOG(__func__);
	int iRet = 0;
	MYSQL_ROW row;

	Reset();
	vecOrderRefundChannelFlowDatas.clear();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" SELECT "
		" mch_id, refund_no, channelid, refund_fee, refund_channel_profit, refundtime "
		" FROM shop_db.refund_channels "
		" WHERE "
		" refund_no = '%s'",
		strRefundNo.c_str());

	CDEBUG_LOG("GetOrderRefundChannelTableFlowDataSql sql: %s", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetOrderRefundChannelTableFlowDataSql Execute Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -20;
	}

	if (sql_instance.num_rows() <= 0)
	{
		// 数据不存在
		return RET_HASNOREC;
	}

	for (int i = 0; i < sql_instance.num_rows(); ++i)
	{
		OrderRefundChannelFlowData orderRefundChannelFlow;
		orderRefundChannelFlow.Reset();
		if ((row = sql_instance.fetch_row()))
		{
			orderRefundChannelFlow.mch_id = (row[0]) ? row[0] : "";
			orderRefundChannelFlow.refund_no = (row[1]) ? row[1] : "";
			orderRefundChannelFlow.channel_id = (row[2]) ? row[2] : "";
			orderRefundChannelFlow.refund_fee = (row[3]) ? atoll(row[3]) : 0;
			orderRefundChannelFlow.refund_channel_profit = (row[4]) ? atoll(row[4]) : 0;
			orderRefundChannelFlow.refund_time = (row[5]) ? atoll(row[5]) : 0;
			vecOrderRefundChannelFlowDatas.push_back(orderRefundChannelFlow);
		}
		else
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg),
				"FetchRow Failed.Ret[%d] Err[%u~%s]",
				iRet, sql_instance.get_errno(), sql_instance.get_error());
			return -30;
		}
	}
	sql_instance.free_result();

	return RET_HASREC;
}


INT32 CPayTransactionFlowDao::InsertPayIdenticalWxToDB(clib_mysql& sql_instance, const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime)
{
	BEGIN_LOG(__func__);

	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into bill_db.t_bill_success_flow_%s (pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit, bill_status) select shop.pay_time, shop.order_no, shop.out_order_no, shop.transaction_id, " 
		" shop.mch_id, shop.channel_id, shop.pay_channel, shop.trade_type, shop.order_status, shop.total_fee, shop.total_commission, shop.shop_amount, shop.refund_fee, shop.refund_no, shop.out_refund_no,  " 
		" shop.refund_id, shop.payment_profit, shop.channel_profit, shop.bm_profit, shop.service_profit, shop.bill_status from bill_db.t_wxpay_flow_%s as wx inner JOIN  bill_db.t_order_wxpay_flow_%s as shop "
		" on shop.order_no = wx.order_no  and (wx.total_fee * 100) = shop.total_fee and shop.order_status = wx.order_status "
		" where shop.pay_time >= '%s' and shop.pay_time <= '%s' and shop.order_status = 'SUCCESS' ",
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertPayIdenticalWxToDB:sql_stmt:[%s].", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertPayIdenticalWxToDB Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
}


INT32 CPayTransactionFlowDao::InsertRefundIdenticalWxToDB(clib_mysql& sql_instance, const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime)
{
	BEGIN_LOG(__func__);

	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into bill_db.t_bill_success_flow_%s (pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit, bill_status) select shop.pay_time, shop.order_no, shop.out_order_no, shop.transaction_id, "
		" shop.mch_id, shop.channel_id, shop.pay_channel, shop.trade_type, shop.order_status, shop.total_fee, shop.total_commission, shop.shop_amount, shop.refund_fee, shop.refund_no, shop.out_refund_no, "
		" shop.refund_id, shop.payment_profit, shop.channel_profit, shop.bm_profit, shop.service_profit, shop.bill_status from bill_db.t_wxpay_flow_%s as wx inner JOIN  bill_db.t_order_wxpay_flow_%s as shop "
		" on shop.order_no = wx.order_no and (wx.refund_fee * 100) = shop.refund_fee and shop.order_status = wx.order_status "
		" where shop.pay_time >= '%s' and shop.pay_time <= '%s' and shop.order_status = 'REFUND' ",
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertRefundIdenticalToDB:sql_stmt:[%s].", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertRefundIdenticalToDB Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
}

//distinct
INT32 CPayTransactionFlowDao::InsertPayDistinctWxToDB(clib_mysql& sql_instance, const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime)
{
	BEGIN_LOG(__func__);

	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into bill_db.t_wx_overflow_%s (pay_time, app_id, mch_id, sub_mch_id, device_info, transaction_id, order_no, openid, trade_type, order_status, bank_type, fee_type, total_fee, red_amount, refund_id, refund_no, "
		" refund_fee, red_refund_amount, refund_type, refund_status, goods_name, shop_packet, counter_fee, rate) SELECT wx.pay_time, wx.app_id, wx.mch_id, wx.sub_mch_id, wx.device_info, wx.transaction_id, wx.order_no,"
		" wx.openid, wx.trade_type, wx.order_status, wx.bank_type, wx.fee_type, wx.total_fee, wx.red_amount, wx.refund_id, wx.refund_no,  wx.refund_fee, wx.red_refund_amount, wx.refund_type, wx.refund_status, wx.goods_name, "
		" wx.shop_packet, wx.counter_fee, wx.rate FROM bill_db.t_wxpay_flow_%s AS wx LEFT JOIN  bill_db.t_order_wxpay_flow_%s AS shop ON shop.order_no = wx.order_no AND (wx.total_fee * 100) = shop.total_fee  AND shop.order_status = wx.order_status "
		" WHERE wx.pay_time >= '%s' AND wx.pay_time <= '%s' AND wx.order_status = 'SUCCESS' AND shop.order_no IS NULL",
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertPayDistinctWxToDB:sql_stmt:[%s].", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertPayDistinctWxToDB Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
}


INT32 CPayTransactionFlowDao::InsertRefundDistinctWxToDB(clib_mysql& sql_instance, const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime)
{
	BEGIN_LOG(__func__);

	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into bill_db.t_wx_overflow_%s (pay_time, app_id, mch_id, sub_mch_id, device_info, transaction_id, order_no, openid, trade_type, order_status, bank_type, fee_type, total_fee, red_amount, refund_id, refund_no, "
		" refund_fee, red_refund_amount, refund_type, refund_status, goods_name, shop_packet, counter_fee, rate) SELECT wx.pay_time, wx.app_id, wx.mch_id, wx.sub_mch_id, wx.device_info, wx.transaction_id, wx.order_no,"
		" wx.openid, wx.trade_type, wx.order_status, wx.bank_type, wx.fee_type, wx.total_fee, wx.red_amount, wx.refund_id, wx.refund_no,  wx.refund_fee, wx.red_refund_amount, wx.refund_type, wx.refund_status, wx.goods_name, "
		" wx.shop_packet, wx.counter_fee, wx.rate FROM bill_db.t_wxpay_flow_%s AS wx LEFT JOIN  bill_db.t_order_wxpay_flow_%s AS shop ON shop.order_no = wx.order_no AND (wx.refund_fee * 100) = shop.refund_fee  AND shop.order_status = wx.order_status"
		" WHERE wx.pay_time >= '%s' AND wx.pay_time <= '%s' AND wx.order_status = 'REFUND' AND shop.order_no IS NULL",
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertRefundDistinctWxToDB:sql_stmt:[%s].", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertRefundDistinctWxToDB Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
}

int CPayTransactionFlowDao::GetPayBillData(clib_mysql& sql_instance,
									const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
									std::map<std::string, OrderPayBillSumary>& orderPayBillSMap)
{
	BEGIN_LOG(__func__);

	int iRet = 0;
	MYSQL_ROW row;

	Reset();

	char sql_stmt[4096];

	snprintf(sql_stmt, sizeof(sql_stmt),
		" SELECT mch_id, SUM(shop_amount), SUM(channel_profit), SUM(service_profit), SUM(payment_profit), SUM(bm_profit), SUM(total_commission), SUM(total_fee), COUNT(*) from bill_db.t_bill_success_flow_%s where order_status = 'SUCCESS' AND "
		" pay_time >= '%s'  AND  pay_time <= '%s' GROUP BY mch_id ",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG("GetPayBillData sql: %s", sql_stmt);

	iRet = sql_instance.query(sql_stmt);

	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetPayBillData Execute Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -20;
	}

	if (sql_instance.num_rows() <= 0)
	{
		// 数据不存在
		return RET_HASNOREC;
	}
	
	for (int i = 0; i < sql_instance.num_rows(); i++)
	{
		OrderPayBillSumary orderPayBill;
		orderPayBill.Reset();
		if ((row = sql_instance.fetch_row()))
		{
			orderPayBill.mch_id = (row[0]) ? row[0] : "";
			orderPayBill.shop_amount = (row[1]) ? atoll(row[1]) : 0;
			orderPayBill.channel_profit = (row[2]) ? atoll(row[2]) : 0;
			orderPayBill.service_profit = (row[3]) ? atoll(row[3]) : 0;
			orderPayBill.payment_profit = (row[4]) ? atoll(row[4]) : 0;
			orderPayBill.bm_profit = (row[5]) ? atoll(row[5]) : 0;
			orderPayBill.total_commission = (row[6]) ? atoll(row[6]) : 0;
			orderPayBill.total_fee = (row[7]) ? atoll(row[7]) : 0;
			orderPayBill.trade_count = (row[8]) ? atoll(row[8]) : 0;

			orderPayBillSMap.insert(std::make_pair(orderPayBill.mch_id, orderPayBill));
		}
		else
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg),
				"FetchRow Failed.Ret[%d] Err[%u~%s]",
				iRet, sql_instance.get_errno(), sql_instance.get_error());
			return -30;
		}
	}
	sql_instance.free_result();
	return RET_HASREC;
}


int CPayTransactionFlowDao::GetRefundBillData(clib_mysql& sql_instance, 
									const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
									std::map<std::string, OrderRefundBillSumary>& orderRefundBillSMap)
{
	BEGIN_LOG(__func__);

	int iRet = 0;
	MYSQL_ROW row;

	Reset();

	char sql_stmt[4096];

	snprintf(sql_stmt, sizeof(sql_stmt),
		" SELECT mch_id, SUM(shop_amount), SUM(channel_profit), SUM(service_profit), SUM(payment_profit), SUM(bm_profit), SUM(total_commission), SUM(refund_fee), COUNT(*) from bill_db.t_bill_success_flow_%s where order_status = 'REFUND' AND "
		" pay_time >= '%s'  AND  pay_time <= '%s' GROUP BY mch_id ",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG("GetRefundBillData sql: %s", sql_stmt);

	iRet = sql_instance.query(sql_stmt);

	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetRefundBillData Execute Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -20;
	}

	if (sql_instance.num_rows() <= 0)
	{
		// 数据不存在
		return RET_HASNOREC;
	}

	for (int i = 0; i < sql_instance.num_rows(); i++)
	{
		OrderRefundBillSumary orderRefundBill;
		orderRefundBill.Reset();
		if ((row = sql_instance.fetch_row()))
		{
			orderRefundBill.mch_id = (row[0]) ? row[0] : "";
			orderRefundBill.shop_amount = (row[1]) ? atoll(row[1]) : 0;
			orderRefundBill.channel_profit = (row[2]) ? atoll(row[2]) : 0;
			orderRefundBill.service_profit = (row[3]) ? atoll(row[3]) : 0;
			orderRefundBill.payment_profit = (row[4]) ? atoll(row[4]) : 0;
			orderRefundBill.bm_profit = (row[5]) ? atoll(row[5]) : 0;
			orderRefundBill.total_commission = (row[6]) ? atoll(row[6]) : 0;
			orderRefundBill.refund_fee = (row[7]) ? atoll(row[7]) : 0;
			orderRefundBill.refund_count = (row[8]) ? atoll(row[8]) : 0;

			orderRefundBillSMap.insert(std::make_pair(orderRefundBill.mch_id, orderRefundBill));
		}
		else
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg),
				"FetchRow Failed.Ret[%d] Err[%u~%s]",
				iRet, sql_instance.get_errno(), sql_instance.get_error());
			return -30;
		}
	}
	sql_instance.free_result();
	return RET_HASREC;
}


int CPayTransactionFlowDao::GetChannelBillData(clib_mysql& sql_instance, const std::string& strTableFix, const std::string& strBmId,
			const std::string& strBeginTime, const std::string& strEndTime, const std::string& order_status, std::map<std::string, int>& channelMap)
{
	BEGIN_LOG(__func__);

	int iRet = 0;
	MYSQL_ROW row;
	Reset();
	//t_order_channel_flow
	char sql_stmt[4096];
	snprintf(sql_stmt, sizeof(sql_stmt),
		" SELECT channel_id, SUM(channel_profit) from bill_db.%s where order_status = '%s' AND "
		" pay_time >= '%s'  AND  pay_time <= '%s' and order_no in (SELECT order_no from bill_db.t_bill_success_flow_%s where pay_time >= '%s' and pay_time <= '%s' GROUP BY order_no) GROUP BY channel_id ",
		strTableFix.c_str(), order_status.c_str(), strBeginTime.c_str(), strEndTime.c_str(),
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG("GetChannelBillData sql: %s", sql_stmt);

	iRet = sql_instance.query(sql_stmt);

	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetChannelBillData Execute Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -20;
	}

	if (sql_instance.num_rows() <= 0)
	{
		// 数据不存在
		return RET_HASNOREC;
	}

	for (int i = 0; i < sql_instance.num_rows(); i++)
	{
		if ((row = sql_instance.fetch_row()))
		{
			std::string channel_id = (row[0]) ? row[0] : "";
			int channel_profit = (row[1]) ? atoll(row[1]) : 0;
			channelMap.insert(std::make_pair(channel_id, channel_profit));
		}
		else
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg),
				"FetchRow Failed.Ret[%d] Err[%u~%s]",
				iRet, sql_instance.get_errno(), sql_instance.get_error());
			return -30;
		}
	}
	sql_instance.free_result();
	return RET_HASREC;
}

INT32 CPayTransactionFlowDao::EmptyTableData(clib_mysql& sql_instance,
									const std::string& TableName, const std::string& strBmid,const std::string& strBeginTime,
									const std::string& strEndTime)
{
	BEGIN_LOG(__func__);

	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" DELETE FROM bill_db.%s_%s "
		" WHERE pay_time >= '%s' AND pay_time <= '%s' ",
		TableName.c_str(), strBmid.c_str(),strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::EmptyTableData:sql_stmt:[%s].", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"EmptyTableData Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
}

INT32 CPayTransactionFlowDao::GetWxOverFlowData(clib_mysql& sql_instance, const std::string& strBmId, const std::string& strBeginTime,
							const std::string& strEndTime, std::vector<WxFlowSummary>& wxOverFlowVec)
{
	BEGIN_LOG(__func__);
	int iRet = 0;
	MYSQL_ROW row;
	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" SELECT "
		" pay_time, transaction_id, order_no, trade_type, "
		" order_status, refund_id, refund_no "
		" FROM bill_db.t_wx_overflow_%s "
		" WHERE "
		" pay_time >= '%s'  AND  pay_time <= '%s'",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG("GetWxOverFlowData sql: %s", sql_stmt);

	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetWxOverFlowData Execute Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -20;
	}

	if (sql_instance.num_rows() <= 0)
	{
		// 数据不存在
		return RET_HASNOREC;
	}

	for (int i = 0; i < sql_instance.num_rows(); ++i)
	{
		WxFlowSummary wxFlowSum;
		wxFlowSum.Reset();
		if ((row = sql_instance.fetch_row()))
		{
			wxFlowSum.pay_time = (row[0]) ? row[0] : "";
			wxFlowSum.transaction_id = (row[1]) ? row[1] : "";
			wxFlowSum.order_no = (row[2]) ? row[2] : "";
			wxFlowSum.trade_type = (row[3]) ? row[3] : "";
			wxFlowSum.order_status = (row[4]) ? row[4] : "";
			wxFlowSum.refund_id = (row[5]) ? row[5] : "";
			wxFlowSum.refund_no  = (row[6]) ? row[6] : "";

			wxOverFlowVec.push_back(wxFlowSum);
		}
		else
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg),
				"FetchRow Failed.Ret[%d] Err[%u~%s]",
				iRet, sql_instance.get_errno(), sql_instance.get_error());
			return -30;
		}
	}
	sql_instance.free_result();

	return RET_HASREC;
}

int CPayTransactionFlowDao::GetAliOverFlowData(clib_mysql& sql_instance,
						const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
						std::vector<AliFlowSummary>& aliOverFlowList)
{
	BEGIN_LOG(__func__);
	int iRet = 0;
	MYSQL_ROW row;
	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" SELECT "
		" transaction_id, order_no, order_status, pay_time, "
		" refund_no "
		" FROM bill_db.t_ali_overflow_%s "
		" WHERE "
		" pay_time >= '%s'  AND  pay_time <= '%s'",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG("GetAliOverFlowData sql: %s", sql_stmt);

	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetAliOverFlowData Execute Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -20;
	}

	if (sql_instance.num_rows() <= 0)
	{
		// 数据不存在
		return RET_HASNOREC;
	}

	for (int i = 0; i < sql_instance.num_rows(); ++i)
	{
		AliFlowSummary ali;
		if ((row = sql_instance.fetch_row()))
		{
			//CDEBUG_LOG("row[0]:%s row[1]:%s row[2]:%s row[3]:%s row[4]:%s", row[0], row[1], row[2], row[3], row[4]);
			ali.transaction_id = row[0] ? row[0] : "";
			ali.order_no = row[1] ? row[1] : "";
			ali.order_status = row[2] ? row[2] : "";
			ali.pay_time = row[3] ? row[3] : "";
			ali.refund_no = row[4] ? row[4] : "";
			CDEBUG_LOG("order_no:%s ", ali.order_no.c_str());
			ali.transaction_id = strTrim(ali.transaction_id);
			ali.order_no = strTrim(ali.order_no);
			ali.order_status = strTrim(ali.order_status);
			ali.pay_time = strTrim(ali.pay_time);
			ali.refund_no = strTrim(ali.refund_no);
			aliOverFlowList.push_back(ali);
		}
		else
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg),
				"FetchRow Failed.Ret[%d] Err[%u~%s]",
				iRet, sql_instance.get_errno(), sql_instance.get_error());
			return -30;
		}
	}
	sql_instance.free_result();

	return RET_HASREC;
}



INT32 CPayTransactionFlowDao::InsertTradeTypeOrderToDB(clib_mysql& sql_instance, const std::string& strBmId, const std::string& pay_channel, const std::string& strTableName)
{
	BEGIN_LOG(__func__);

	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into bill_db.%s_%s (pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit, bill_status) select pay_time, order_no, out_order_no, transaction_id, "
		" mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, refund_fee, refund_no, out_refund_no,  "
		" refund_id, payment_profit, channel_profit, bm_profit, service_profit, bill_status from bill_db.t_order_all_flow_%s "
		" where pay_channel = '%s'",
		strTableName.c_str(), strBmId.c_str(),strBmId.c_str(), pay_channel.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertTradeTypeOrderToDB:sql_stmt:[%s].", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertTradeTypeOrderToDB Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
}

INT32 CPayTransactionFlowDao::InsertTradeTypeOrderChannelToDB(clib_mysql& sql_instance, const std::string& strBmId, const std::string& pay_channel, const std::string& strTableName)
{
	BEGIN_LOG(__func__);

	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into bill_db.%s_%s (pay_time, order_no, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, refund_fee, refund_no, channel_profit_rate, channel_profit)"
		" select pay_time, order_no, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, refund_fee, refund_no, channel_profit_rate, channel_profit "
		" from bill_db.t_order_channel_all_flow_%s "
		" where pay_channel = '%s'",
		strTableName.c_str(),strBmId.c_str(), strBmId.c_str(), pay_channel.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertTradeTypeOrderChannelToDB:sql_stmt:[%s].", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertTradeTypeOrderChannelToDB Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();
	return sql_instance.affected_rows();
}


INT32 CPayTransactionFlowDao::TruncateEveryPaymentTypeSysFlowData(clib_mysql& sql_instance, const std::string& strBmId)
{
	BEGIN_LOG(__func__);
	Reset();
	int iRet = 0;
	std::string strWxOrderSql = "TRUNCATE bill_db.t_order_wxpay_flow_" + strBmId;
	std::string strAliOrderSql = "TRUNCATE bill_db.t_order_alipay_flow_" + strBmId;
	std::string strWxOrderChannelSql = "TRUNCATE bill_db.t_order_channel_wxpay_flow_" + strBmId;
	std::string strAliOrderChannelSql = "TRUNCATE bill_db.t_order_channel_alipay_flow_" + strBmId;
	//开启事务
	iRet = sql_instance.query("START TRANSACTION");
	iRet = sql_instance.query(strWxOrderSql.c_str());
	iRet = sql_instance.query(strAliOrderSql.c_str());
	iRet = sql_instance.query(strWxOrderChannelSql.c_str());
	iRet = sql_instance.query(strAliOrderChannelSql.c_str());
	iRet = sql_instance.query("COMMIT");//提交事务
	CDEBUG_LOG("START TRANSACTION\n");
	CDEBUG_LOG("wx_order_sql:[%s]\n", strWxOrderSql.c_str());
	CDEBUG_LOG("ali_order_sql:[%s]\n", strAliOrderSql.c_str());
	CDEBUG_LOG("wx_order_channel_sql:[%s]\n", strWxOrderChannelSql.c_str());
	CDEBUG_LOG("ali_order_channel_sql:[%s]\n", strAliOrderChannelSql.c_str());
	CDEBUG_LOG("COMMIT\n");
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"TruncateEveryPaymentTypeSysFlowData Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
}


INT32 CPayTransactionFlowDao::InsertAliPayIdenticalToDB(clib_mysql& sql_instance,
									const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime, const std::string& order_status)
{
	BEGIN_LOG(__func__);

	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into bill_db.t_bill_success_flow_%s (pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit, bill_status) select shop.pay_time, shop.order_no, shop.out_order_no, shop.transaction_id, "
		" shop.mch_id, shop.channel_id, shop.pay_channel, shop.trade_type, shop.order_status, shop.total_fee, shop.total_commission, shop.shop_amount, shop.refund_fee, shop.refund_no, shop.out_refund_no,  "
		" shop.refund_id, shop.payment_profit, shop.channel_profit, shop.bm_profit, shop.service_profit, shop.bill_status from bill_db.t_alipay_flow_%s as ali inner JOIN  bill_db.t_order_alipay_flow_%s as shop "
		" on shop.order_no = ali.order_no  and ABS(ali.total_fee * 100) = shop.total_fee  and shop.order_status = ali.order_status "
		" where shop.pay_time >= '%s' and shop.pay_time <= '%s' and shop.order_status = '%s'",  //SUCCESS
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str(), order_status.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertAliPayIdenticalToDB:sql_stmt:[%s].", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertAliPayIdenticalToDB Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
}


INT32 CPayTransactionFlowDao::InsertAliPayIdenticalRefundToDB(clib_mysql& sql_instance,
	const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime)
{
	BEGIN_LOG(__func__);

	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into bill_db.t_bill_success_flow_%s (pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit, bill_status) select shop.pay_time, shop.order_no, shop.out_order_no, shop.transaction_id, "
		" shop.mch_id, shop.channel_id, shop.pay_channel, shop.trade_type, shop.order_status, shop.total_fee, shop.total_commission, shop.shop_amount, shop.refund_fee, shop.refund_no, shop.out_refund_no,  "
		" shop.refund_id, shop.payment_profit, shop.channel_profit, shop.bm_profit, shop.service_profit, shop.bill_status from bill_db.t_alipay_flow_%s as ali inner JOIN  bill_db.t_order_alipay_flow_%s as shop "
		" on shop.order_no = ali.order_no  and ali.refund_no = shop.refund_no and ABS(ali.total_fee * 100) = shop.refund_fee and shop.order_status = ali.order_status "
		" where shop.pay_time >= '%s' and shop.pay_time <= '%s' and shop.order_status = 'REFUND'",  //SUCCESS
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertAliPayIdenticalRefundToDB:sql_stmt:[%s].", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertAliPayIdenticalToDB Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
}

INT32 CPayTransactionFlowDao::InsertAliPayDistinctToDB(clib_mysql& sql_instance, const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime, const std::string& order_status)
{
	BEGIN_LOG(__func__);

	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into bill_db.t_ali_overflow_%s (transaction_id, order_no, order_status, goods_name, create_time, pay_time, store_no, store_name, operator_name, terminal_no, each_account, total_fee, shop_net_receipts, alipay_red_fee, score_amount, alipay_discount_amount, "
		" shop_discount_amount, coupon_write_off_fee, coupon_name, shop_red_fee, card_consume_fee, refund_no, service_profit, net_paid_in, mch_id, trade_mode, remark) SELECT ali.transaction_id, ali.order_no, ali.order_status, ali.goods_name, ali.create_time, ali.pay_time, "
		" ali.store_no, ali.store_name, ali.operator_name, ali.terminal_no, ali.each_account, ali.total_fee, ali.shop_net_receipts, ali.alipay_red_fee, ali.score_amount, ali.alipay_discount_amount,  ali.shop_discount_amount, ali.coupon_write_off_fee, ali.coupon_name,  "
		" ali.shop_red_fee, ali.card_consume_fee, ali.refund_no, ali.service_profit, ali.net_paid_in, ali.mch_id, ali.trade_mode, ali.remark FROM bill_db.t_alipay_flow_%s AS ali LEFT JOIN  bill_db.t_order_alipay_flow_%s AS shop ON shop.order_no = ali.order_no "
		" AND ABS(ali.total_fee * 100) = shop.total_fee  AND shop.order_status = ali.order_status WHERE ali.pay_time >= '%s' AND ali.pay_time <= '%s' AND ali.order_status = '%s' AND shop.order_no IS NULL",//SUCCESS
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str(), order_status.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertAliPayDistinctToDB:sql_stmt:[%s].", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertAliPayDistinctToDB Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
}


INT32 CPayTransactionFlowDao::InsertAliPayDistinctRefundToDB(clib_mysql& sql_instance, const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime)
{
	BEGIN_LOG(__func__);

	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into bill_db.t_ali_overflow_%s (transaction_id, order_no, order_status, goods_name, create_time, pay_time, store_no, store_name, operator_name, terminal_no, each_account, total_fee, shop_net_receipts, alipay_red_fee, score_amount, alipay_discount_amount, "
		" shop_discount_amount, coupon_write_off_fee, coupon_name, shop_red_fee, card_consume_fee, refund_no, service_profit, net_paid_in, mch_id, trade_mode, remark) SELECT ali.transaction_id, ali.order_no, ali.order_status, ali.goods_name, ali.create_time, ali.pay_time, "
		" ali.store_no, ali.store_name, ali.operator_name, ali.terminal_no, ali.each_account, ali.total_fee, ali.shop_net_receipts, ali.alipay_red_fee, ali.score_amount, ali.alipay_discount_amount,  ali.shop_discount_amount, ali.coupon_write_off_fee, ali.coupon_name,  "
		" ali.shop_red_fee, ali.card_consume_fee, ali.refund_no, ali.service_profit, ali.net_paid_in, ali.mch_id, ali.trade_mode, ali.remark FROM bill_db.t_alipay_flow_%s AS ali LEFT JOIN  bill_db.t_order_alipay_flow_%s AS shop ON shop.order_no = ali.order_no "
		" AND ABS(ali.total_fee * 100) = shop.refund_fee  and ali.refund_no = shop.refund_no AND shop.order_status = ali.order_status WHERE ali.pay_time >= '%s' AND ali.pay_time <= '%s' AND ali.order_status = 'REFUND' AND shop.order_no IS NULL",//SUCCESS
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertAliPayDistinctRefundToDB:sql_stmt:[%s].", sql_stmt);
	iRet = sql_instance.query(sql_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertAliPayDistinctRefundToDB Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
}

INT32 CPayTransactionFlowDao::RemoveAlipayDifferenceSuccState(clib_mysql& sql_instance, const std::string& strBmId)
{
	BEGIN_LOG(__func__);
	Reset();
	int iRet = 0;
	char sql_succ_stmt[1024];
	snprintf(sql_succ_stmt, sizeof(sql_succ_stmt), "UPDATE bill_db.t_alipay_flow_%s set order_status = 'SUCCESS' where order_status = '交易'", strBmId.c_str());
	
	//开启事务
	iRet = sql_instance.query(sql_succ_stmt);
	CDEBUG_LOG("sql_succ_stmt:[%s]\n", sql_succ_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"RemoveAlipayDifferenceSuccState Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
}

INT32 CPayTransactionFlowDao::RemoveAlipayDifferenceRefundState(clib_mysql& sql_instance, const std::string& strBmId)
{
	BEGIN_LOG(__func__);
	Reset();
	int iRet = 0;
	char sql_refund_stmt[1024];
	snprintf(sql_refund_stmt, sizeof(sql_refund_stmt), "UPDATE bill_db.t_alipay_flow_%s set order_status = 'REFUND' where order_status = '退款'", strBmId.c_str());

	iRet = sql_instance.query(sql_refund_stmt);
	CDEBUG_LOG("sql_refund_stmt:[%s]\n", sql_refund_stmt);
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"RemoveAlipayDifferenceRefundState Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
}
