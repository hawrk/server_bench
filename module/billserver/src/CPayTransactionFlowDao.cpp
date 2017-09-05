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
		" channel_profit, service_profit, bm_profit, total_commission, pay_time,"
		" fee_type,sub_body,shop_calc_rate "
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
			orderFlow.fee_type = (row[17]) ? row[17] :"";
			orderFlow.sub_body = (row[18]) ? row[18] :"";
			orderFlow.shop_calc_rate = (row[19]) ? atol(row[19]): 0;

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

/*
 *
 *	Reconciliation
 */


INT32 CPayTransactionFlowDao::InsertPayIdenticalWxToDB(clib_mysql& sql_instance, const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime)
{
	BEGIN_LOG(__func__);

	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into bill_db.t_bill_success_flow (bm_id,pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit,sub_body, bill_status,settle_status) select shop.bm_id,shop.pay_time, shop.order_no, shop.out_order_no, shop.transaction_id, "
		" shop.mch_id, shop.channel_id, shop.pay_channel, shop.trade_type, shop.order_status, shop.total_fee, shop.total_commission, shop.shop_amount, shop.refund_fee, shop.refund_no, shop.out_refund_no,  " 
		" shop.refund_id, shop.payment_profit, shop.channel_profit, shop.bm_profit, shop.service_profit,shop.sub_body, shop.bill_status,1 from bill_db.t_wxpay_flow as wx inner JOIN  bill_db.t_order_wxpay_flow as shop "
		" on shop.bm_id = wx.bm_id and shop.order_no = wx.order_no  and (wx.total_fee * 100) = shop.total_fee and shop.order_status = wx.order_status "
		" where shop.bm_id = '%s' and shop.pay_time >= '%s' and shop.pay_time <= '%s' and shop.order_status = 'SUCCESS' ",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

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
		" insert into bill_db.t_bill_success_flow (bm_id,pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit,sub_body, bill_status,settle_status) select shop.bm_id,shop.pay_time, shop.order_no, shop.out_order_no, shop.transaction_id, "
		" shop.mch_id, shop.channel_id, shop.pay_channel, shop.trade_type, shop.order_status, shop.total_fee, shop.total_commission, shop.shop_amount, shop.refund_fee, shop.refund_no, shop.out_refund_no, "
		" shop.refund_id, shop.payment_profit, shop.channel_profit, shop.bm_profit, shop.service_profit, shop.sub_body,shop.bill_status,1 from bill_db.t_wxpay_flow as wx inner JOIN  bill_db.t_order_wxpay_flow as shop "
		" on shop.bm_id = wx.bm_id and shop.order_no = wx.order_no and (wx.refund_fee * 100) = shop.refund_fee and shop.order_status = wx.order_status AND shop.refund_no = wx.refund_no"
		" where shop.bm_id = '%s' and shop.pay_time >= '%s' and shop.pay_time <= '%s' and shop.order_status = 'REFUND' ",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

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

//金额不符
INT32 CPayTransactionFlowDao::InsertAmountNotMatchToDB(clib_mysql& sql_instance,
					const std::string& strBmId, const std::string& strPayChannel,
					const std::string& strBillDate,const std::string& strBatchNo,
					const std::string& strBeginTime, const std::string& strEndTime)
{
	int iRet = 0;

	Reset();

	ostringstream sqlss;

	if(strPayChannel == "WXPAY")
	{
	    sqlss.str("");
	    sqlss << "insert into "
	    	  <<BILL_DB<<"."<<BILL_ABNORMAL
			  <<" (bm_id,bill_date, bill_batch_no, pay_channel, cur_type, mch_id, "
			  <<" pf_trade_amount, pf_order_no, ch_trade_amount, ch_order_no, trade_date, abnormal_type, porcess_status)"
			  <<" select shop.bm_id,'"<<strBillDate<<"','"<<strBatchNo<<"',shop.pay_channel,'CNY',shop.mch_id,"
			  <<" shop.total_fee,shop.order_no,(ch.total_fee)*100,ch.order_no,shop.pay_time,'4','0' "
			  <<" from "<<BILL_DB<<"."<<BILL_WXPAY_FLOW<<" as ch INNER JOIN "<<BILL_DB<<"."<<ORDER_WXPAY_FLOW
			  <<" as shop ON shop.bm_id = ch.bm_id AND shop.order_no = ch.order_no AND (ch.total_fee * 100) <> shop.total_fee"
			  <<" AND shop.order_status = ch.order_status WHERE shop.bm_id = '"<<strBmId<<"' AND shop.pay_time >= '"<<strBeginTime
			  <<"' AND shop.pay_time <= '"<<strEndTime<<"' AND shop.order_status = 'SUCCESS'";


		CDEBUG_LOG(" CPayTransactionFlowDao::InsertAmountNotMatchToDB success :sql_stmt:[%s].", sqlss.str().c_str());
		iRet = sql_instance.query(sqlss.str().c_str());
		if (iRet != 0)
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg),
				"InsertAmountNotMatchToDB Execute"
				"Failed.Ret[%d] Err[%u~%s]",
				iRet,
				sql_instance.get_errno(),
				sql_instance.get_error());
			return -20;
		}

		//退款
	    sqlss.str("");
	    sqlss << "insert into "
	    	  <<BILL_DB<<"."<<BILL_ABNORMAL
			  <<" (bm_id,bill_date, bill_batch_no, pay_channel, cur_type, mch_id, "
			  <<" pf_refund_amount, pf_refund_no, ch_refund_amount, ch_refund_no, trade_date, abnormal_type, porcess_status)"
			  <<" select shop.bm_id,'"<<strBillDate<<"','"<<strBatchNo<<"',shop.pay_channel,ch.fee_type,shop.mch_id,"
			  <<" shop.refund_fee,shop.refund_no,(ch.refund_fee)*100,ch.refund_no,shop.pay_time,'4','0' "
			  <<" from "<<BILL_DB<<"."<<BILL_WXPAY_FLOW<<" as ch INNER JOIN "<<BILL_DB<<"."<<ORDER_WXPAY_FLOW
			  <<" as shop ON shop.bm_id = ch.bm_id AND shop.order_no = ch.order_no AND (ch.refund_fee * 100) <> shop.refund_fee"
			  <<" AND shop.order_status = ch.order_status AND shop.refund_no = ch.refund_no WHERE shop.bm_id = '"<<strBmId
			  <<"' AND shop.pay_time >= '"<<strBeginTime<<"' AND shop.pay_time <= '"<<strEndTime
			  <<"' AND shop.order_status = 'REFUND'";

		CDEBUG_LOG(" CPayTransactionFlowDao::InsertAmountNotMatchToDB refund :sql_stmt:[%s].", sqlss.str().c_str());
		iRet = sql_instance.query(sqlss.str().c_str());
		if (iRet != 0)
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg),
				"InsertAmountNotMatchToDB Execute"
				"Failed.Ret[%d] Err[%u~%s]",
				iRet,
				sql_instance.get_errno(),
				sql_instance.get_error());
			return -20;
		}

	}
	else if(strPayChannel == "ALIPAY")
	{
	    sqlss.str("");
	    sqlss << "insert into "
	    	  <<BILL_DB<<"."<<BILL_ABNORMAL
			  <<" (bm_id,bill_date, bill_batch_no, pay_channel, cur_type, mch_id, "
			  <<" pf_trade_amount, pf_order_no, ch_trade_amount, ch_order_no, trade_date, abnormal_type, porcess_status)"
			  <<" select shop.bm_id,'"<<strBillDate<<"','"<<strBatchNo<<"',shop.pay_channel,'CNY',shop.mch_id,"
			  <<" shop.total_fee,shop.order_no,(ch.total_fee)*100,ch.order_no,shop.pay_time,'4','0' "
			  <<" from "<<BILL_DB<<"."<<BILL_ALIPAY_FLOW<<" as ch INNER JOIN "<<BILL_DB<<"."<<ORDER_ALIPAY_FLOW
			  <<" as shop ON shop.bm_id = ch.bm_id AND shop.order_no = ch.order_no AND (ch.total_fee * 100) <> shop.total_fee"
			  <<" AND shop.order_status = ch.order_status WHERE shop.bm_id = '"<<strBmId<<"' AND shop.pay_time >= '"<<strBeginTime
			  <<"' AND shop.pay_time <= '"<<strEndTime<<"' AND shop.order_status = 'SUCCESS'";


		CDEBUG_LOG(" CPayTransactionFlowDao::InsertAmountNotMatchToDB success :sql_stmt:[%s].", sqlss.str().c_str());
		iRet = sql_instance.query(sqlss.str().c_str());
		if (iRet != 0)
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg),
				"InsertAmountNotMatchToDB Execute"
				"Failed.Ret[%d] Err[%u~%s]",
				iRet,
				sql_instance.get_errno(),
				sql_instance.get_error());
			return -20;
		}

		//退款
	    sqlss.str("");
	    sqlss << "insert into "
	    	  <<BILL_DB<<"."<<BILL_ABNORMAL
			  <<" (bm_id,bill_date, bill_batch_no, pay_channel, cur_type, mch_id, "
			  <<" pf_refund_amount, pf_refund_no, ch_refund_amount, ch_refund_no, trade_date, abnormal_type, porcess_status)"
			  <<" select shop.bm_id,'"<<strBillDate<<"','"<<strBatchNo<<"',shop.pay_channel,'CNY',shop.mch_id,"
			  <<" shop.refund_fee,shop.refund_no,(ch.total_fee)*100,ch.refund_no,shop.pay_time,'4','0' "
			  <<" from "<<BILL_DB<<"."<<BILL_ALIPAY_FLOW<<" as ch INNER JOIN "<<BILL_DB<<"."<<ORDER_ALIPAY_FLOW
			  <<" as shop ON shop.bm_id = ch.bm_id AND shop.order_no = ch.order_no AND (ch.total_fee * 100) <> shop.refund_fee"
			  <<" AND shop.order_status = ch.order_status AND shop.refund_no = ch.refund_no WHERE shop.bm_id = '"<<strBmId
			  <<"' AND shop.pay_time >= '"<<strBeginTime<<"' AND shop.pay_time <= '"<<strEndTime
			  <<"' AND shop.order_status = 'REFUND'";

		CDEBUG_LOG(" CPayTransactionFlowDao::InsertAmountNotMatchToDB refund :sql_stmt:[%s].", sqlss.str().c_str());
		iRet = sql_instance.query(sqlss.str().c_str());
		if (iRet != 0)
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg),
				"InsertAmountNotMatchToDB Execute"
				"Failed.Ret[%d] Err[%u~%s]",
				iRet,
				sql_instance.get_errno(),
				sql_instance.get_error());
			return -20;
		}
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
		" insert into bill_db.t_wx_overflow (bm_id,pay_time, app_id, mch_id, sub_mch_id, device_info, transaction_id, order_no, openid, trade_type, order_status, bank_type, fee_type, total_fee, red_amount, refund_id, refund_no, "
		" refund_fee, red_refund_amount, refund_type, refund_status, goods_name, shop_packet, counter_fee, rate,overflow_type) SELECT wx.bm_id,wx.pay_time, wx.app_id, wx.mch_id, wx.sub_mch_id, wx.device_info, wx.transaction_id, wx.order_no,"
		" wx.openid, wx.trade_type, wx.order_status, wx.bank_type, wx.fee_type, wx.total_fee*100, wx.red_amount*100, wx.refund_id, wx.refund_no,  wx.refund_fee*100, wx.red_refund_amount*100, wx.refund_type, wx.refund_status, wx.goods_name, "
		" wx.shop_packet, wx.counter_fee*100, wx.rate ,'2' FROM bill_db.t_wxpay_flow AS wx LEFT JOIN  bill_db.t_order_wxpay_flow AS shop ON shop.bm_id = wx.bm_id and shop.order_no = wx.order_no  "
		" AND shop.order_status = wx.order_status WHERE  wx.bm_id = '%s' and wx.pay_time >= '%s' AND wx.pay_time <= '%s' AND wx.order_status = 'SUCCESS' AND shop.order_no IS NULL",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

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

INT32 CPayTransactionFlowDao::InsertPaySuccessToDB(clib_mysql& sql_instance, const std::string& strBmId,
		const std::string& strBeginTime, const std::string& strEndTime,const std::string& strPayChannel)
{
	int iRet = 0;

	Reset();
	string overflow_table,order_flow_table,channel_flow;

	if(strPayChannel == "WXPAY")
	{
		overflow_table = BILL_WX_OVERFLOW;
		order_flow_table = ORDER_WXPAY_FLOW;
		channel_flow = BILL_WXPAY_FLOW;
	}
	else if(strPayChannel == "ALIPAY")
	{
		overflow_table = BILL_ALI_OVERFLOW;
		order_flow_table = ORDER_ALIPAY_FLOW;
		channel_flow = BILL_ALIPAY_FLOW;
	}

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		"INSERT INTO bill_db.%s (bm_id,pay_time,mch_id,transaction_id,order_no,"
		"trade_type,order_status,total_fee,refund_id,refund_no,refund_fee,overflow_type"
		") SELECT shop.bm_id,shop.pay_time,shop.mch_id,shop.transaction_id,shop.order_no,"
		"shop.trade_type,shop.order_status,shop.total_fee,shop.refund_id,shop.refund_no,"
		"shop.refund_fee,'1' FROM bill_db.%s AS shop "
		"LEFT JOIN bill_db.%s AS wx  ON shop.bm_id = wx.bm_id "
		"AND shop.order_no = wx.order_no "
		"AND shop.order_status = wx.order_status WHERE shop.bm_id = '%s'"
		"AND shop.pay_time >= '%s' "
		"AND shop.pay_time <= '%s' "
		"AND shop.order_status = 'SUCCESS' "
		"AND wx.order_no IS NULL",
		overflow_table.c_str(),order_flow_table.c_str(),channel_flow.c_str(),
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	// AND (wx.total_fee * 100) = shop.total_fee

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

INT32 CPayTransactionFlowDao::InsertPayRefundToDB(clib_mysql& sql_instance, const std::string& strBmId,
		const std::string& strBeginTime, const std::string& strEndTime,const std::string& paychannel)
{
	int iRet = 0;

	Reset();

	string overflow_table,order_flow_table,channel_flow;

	if(paychannel == "WXPAY")
	{
		overflow_table = BILL_WX_OVERFLOW;
		order_flow_table = ORDER_WXPAY_FLOW;
		channel_flow = BILL_WXPAY_FLOW;
	}
	else if(paychannel == "ALIPAY")
	{
		overflow_table = BILL_ALI_OVERFLOW;
		order_flow_table = ORDER_ALIPAY_FLOW;
		channel_flow = BILL_ALIPAY_FLOW;
	}

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		"INSERT INTO bill_db.%s (bm_id,pay_time,mch_id,transaction_id,order_no,"
		"trade_type,order_status,total_fee,refund_id,refund_no,refund_fee,overflow_type"
		") SELECT shop.bm_id,shop.pay_time,shop.mch_id,shop.transaction_id,shop.order_no,"
		"shop.trade_type,shop.order_status,shop.total_fee,shop.refund_id,shop.refund_no,"
		"shop.refund_fee,'1' FROM bill_db.%s AS shop "
		"LEFT JOIN bill_db.%s AS wx  ON shop.bm_id = wx.bm_id "
		"AND shop.order_no = wx.order_no  "
		"AND shop.order_status = wx.order_status AND shop.refund_no = wx.refund_no "
		"WHERE shop.bm_id = '%s'"
		"AND shop.pay_time >= '%s' "
		"AND shop.pay_time <= '%s' "
		"AND shop.order_status = 'REFUND' "
		"AND wx.order_no IS NULL",
		overflow_table.c_str(),order_flow_table.c_str(),channel_flow.c_str(),
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	//AND (wx.refund_fee * 100) = shop.refund_fee

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertPayRefundToDB:sql_stmt:[%s].", sql_stmt);
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
		" insert into bill_db.t_wx_overflow (bm_id,pay_time, app_id, mch_id, sub_mch_id, device_info, transaction_id, order_no, openid, trade_type, order_status, bank_type, fee_type, total_fee, red_amount, refund_id, refund_no, "
		" refund_fee, red_refund_amount, refund_type, refund_status, goods_name, shop_packet, counter_fee, rate,overflow_type) SELECT wx.bm_id,wx.pay_time, wx.app_id, wx.mch_id, wx.sub_mch_id, wx.device_info, wx.transaction_id, wx.order_no,"
		" wx.openid, wx.trade_type, wx.order_status, wx.bank_type, wx.fee_type, wx.total_fee, wx.red_amount, wx.refund_id, wx.refund_no,  wx.refund_fee, wx.red_refund_amount, wx.refund_type, wx.refund_status, wx.goods_name, "
		" wx.shop_packet, wx.counter_fee, wx.rate,'2' FROM bill_db.t_wxpay_flow AS wx LEFT JOIN  bill_db.t_order_wxpay_flow AS shop ON shop.bm_id = wx.bm_id and shop.order_no = wx.order_no  "
		" AND shop.order_status = wx.order_status AND shop.refund_no = wx.refund_no WHERE wx.bm_id = '%s' and wx.pay_time >= '%s' AND wx.pay_time <= '%s' AND wx.order_status = 'REFUND' AND shop.order_no IS NULL",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	//AND (wx.refund_fee * 100) = shop.refund_fee

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

int CPayTransactionFlowDao::GetCheckedBillData(clib_mysql& sql_instance,const std::string& strBmId, const std::string strChannel,
					const std::string& strBeginTime, const std::string& strEndTime,std::vector<CheckedBillData>& vecbilldata)
{
	BEGIN_LOG(__func__);

	int iRet = 0;
	MYSQL_ROW row;

	Reset();

	char sql_stmt[4096];

	snprintf(sql_stmt, sizeof(sql_stmt),
		" select bm_id,pay_time,order_no,out_order_no,transaction_id,mch_id,channel_id,pay_channel,"
		"trade_type, order_status,total_fee,total_commission,shop_amount,"
		" refund_fee,refund_no, out_refund_no,refund_id,payment_profit,channel_profit,bm_profit,service_profit,sub_body"
		" from bill_db.t_bill_success_flow where bm_id = '%s' and pay_time >= '%s'  AND  pay_time <= '%s' and pay_channel = '%s'",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str(),strChannel.c_str());

	CDEBUG_LOG("GetCheckedBillData sql: %s", sql_stmt);

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
		CheckedBillData billdata;
		billdata.Reset();
		if ((row = sql_instance.fetch_row()))
		{
			billdata.bm_id = (row[0]) ? row[0] : "";
			billdata.pay_time = (row[1]) ? row[1] : "";
			billdata.order_no = (row[2]) ? row[2] : "";
			billdata.out_order_no = (row[3]) ? row[3] : "";
			billdata.transaction_id = (row[4]) ? row[4] : "";
			billdata.mch_id = (row[5]) ? row[5] : "";
			billdata.channel_id = (row[6]) ? row[6] : "";
			billdata.pay_channel = (row[7]) ? row[7] : "";
			billdata.trade_type = (row[8]) ? row[8] : "";
			billdata.order_status = (row[9]) ? row[9] : "";
			billdata.total_fee = (row[10]) ? row[10] : "";
			billdata.total_commission = (row[11]) ? row[11] : "";
			billdata.shop_amount = (row[12]) ? row[12] : "";
			billdata.refund_fee = (row[13]) ? row[13] : "";
			billdata.refund_no = (row[14]) ? row[14] : "";
			billdata.out_refund_no = (row[15]) ? row[15] : "";
			billdata.refund_id = (row[16]) ? row[16] : "";
			billdata.payment_profit = (row[17]) ? row[17] : "";
			billdata.channel_profit = (row[18]) ? row[18] : "";
			billdata.bm_profit = (row[19]) ? row[19] : "";
			billdata.service_profit = (row[20]) ? row[20] : "";
			billdata.sub_body = (row[21]) ? row[21] : "";

			vecbilldata.push_back(billdata);
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


int CPayTransactionFlowDao::GetPayBillData(clib_mysql& sql_instance,
									const std::string& strBmId, const std::string strChannel,const std::string& strBeginTime, const std::string& strEndTime,
									std::map<std::string, OrderPayBillSumary>& orderPayBillSMap)
{
	BEGIN_LOG(__func__);

	int iRet = 0;
	MYSQL_ROW row;

	Reset();

	char sql_stmt[4096];

	snprintf(sql_stmt, sizeof(sql_stmt),
		" SELECT mch_id, SUM(shop_amount), SUM(channel_profit), SUM(service_profit), SUM(payment_profit), SUM(bm_profit), SUM(total_commission), SUM(total_fee), COUNT(*) "
		" from bill_db.t_bill_success_flow where bm_id = '%s' and pay_time >= '%s'  AND  pay_time <= '%s' and pay_channel = '%s'"
		" and order_status = 'SUCCESS' GROUP BY mch_id",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str(),strChannel.c_str());

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
									const std::string& strBmId, const std::string strChannel,const std::string& strBeginTime, const std::string& strEndTime,
									std::map<std::string, OrderRefundBillSumary>& orderRefundBillSMap)
{
	BEGIN_LOG(__func__);

	int iRet = 0;
	MYSQL_ROW row;

	Reset();

	char sql_stmt[4096];

	snprintf(sql_stmt, sizeof(sql_stmt),
		" SELECT mch_id, SUM(shop_amount), SUM(channel_profit), SUM(service_profit), SUM(payment_profit), SUM(bm_profit), SUM(total_commission), SUM(refund_fee), COUNT(*) "
		" from bill_db.t_bill_success_flow where bm_id = '%s' and pay_time >= '%s'  AND  pay_time <= '%s' and pay_channel = '%s' and "
		"order_status = 'REFUND' GROUP BY mch_id",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str(),strChannel.c_str());

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


int CPayTransactionFlowDao::GetChannelBillData(clib_mysql& sql_instance, const std::string& strTableFix, const std::string& strBmId,const std::string& Chann,
			const std::string& strBeginTime, const std::string& strEndTime, const std::string& order_status,std::map<std::string, OrderChannelFlowData>& channelMap)
{
	BEGIN_LOG(__func__);

	int iRet = 0;
	MYSQL_ROW row;
	Reset();
	//t_order_channel_flow
	char sql_stmt[4096];
	snprintf(sql_stmt, sizeof(sql_stmt),
		" SELECT channel_id, count(*),SUM(total_fee),SUM(refund_fee),SUM(channel_profit) from bill_db.%s where bm_id = '%s' and  "
		" pay_time >= '%s'  AND  pay_time <= '%s' and pay_channel = '%s' and order_status = '%s' and order_no in "
		"(SELECT order_no from bill_db.t_bill_success_flow where bm_id ='%s' and pay_time >= '%s' and pay_time <= '%s' and pay_channel = '%s' "
		"GROUP BY order_no) GROUP BY channel_id ",
		strTableFix.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str(),Chann.c_str(),order_status.c_str(),
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str(),Chann.c_str());

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
		OrderChannelFlowData channelflowdata;
		channelflowdata.Reset();
		if ((row = sql_instance.fetch_row()))
		{
			std::string channel_id = (row[0]) ? row[0] : "";
			channelflowdata.total_count = (row[1]) ? atoi(row[1]) : 0;
			channelflowdata.total_fee = (row[2] ? atoi(row[2]):0);
			channelflowdata.refund_fee = (row[3] ? atoi(row[3]):0);
			channelflowdata.channel_profit = (row[4] ? atoi(row[4]):0);

			channelMap.insert(std::make_pair(channel_id, channelflowdata));
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

INT32 CPayTransactionFlowDao::InsertDistributionDB(clib_mysql& sql_instance,const std::string& strBmId,const std::string& bill_date,const std::string& batch_no,
						const std::string& pay_channel,const std::string& channel_code,OrderStat& ordStat,TRemitBill& remitBill,const char* fund_type)
{
	int iRet = 0;
	ostringstream sqlss;
    sqlss.str("");
    sqlss << "insert into "
    	  <<BILL_DB<<"."<<BILL_DISTRIBUTION
		  <<" (bm_id,bill_date,bill_batch_no,mch_id,mch_name,pay_channel,trade_count,trade_amount,refund_count,"
		  <<"refund_amount,cost_fee,profit,unsettle,share_profit,fund_type,org_id)"
		  <<" values('"<<strBmId<<"','"<<bill_date<<"','"<<batch_no<<"','"<<ordStat.mch_id<<"','"<<remitBill.sShopName<<"','"<<pay_channel<<"','"
		  <<ordStat.trade_count<<"','"<<ordStat.trade_amount<<"','"
		  <<ordStat.refund_count<<"','"<<ordStat.refund_amount<<"','"<<ordStat.payment_net_profit<<"','"
		  <<ordStat.trade_net_amount<<"','"<<ordStat.shop_net_amount<<"','"
		  <<ordStat.shared_profit<<"','"<<fund_type<<"','"<<channel_code<<"');";

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertDistributionDB:sql_stmt:[%s].", sqlss.str().c_str());
	iRet = sql_instance.query(sqlss.str().c_str());
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertDistributionDB Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
}

INT32 CPayTransactionFlowDao::InsertSettleDB(clib_mysql& sql_instance,const std::string& strBmId,const std::string& bill_date,const std::string& batch_no,
				const std::string& pay_channel,TRemitBill& remitBill)
{
	int iRet = 0;
	ostringstream sqlss;
    sqlss.str("");
    sqlss << "insert into "
    	  <<BILL_DB<<"."<<BILL_SETTLE
		  <<" (bm_id,bill_date,batch_no,card_type,card_name,bank_cardno,bank_type,bank_flag,bank_inscode,fund_type,"
		  <<"partner_id,partner_name,pay_channel,settle_amt,settle_status)"
		  <<" values('"<<strBmId<<"','"<<bill_date<<"','"<<batch_no<<"','"<<remitBill.sBankCardType<<"','"<<remitBill.sName<<"','"
		  <<remitBill.sBankCardNo<<"','"<<remitBill.sBankType<<"','"<<remitBill.sBankFlag<<"','"
		  <<remitBill.sBranchNo<<"','"<<remitBill.sType<<"','"
		  <<remitBill.account_id<<"','"<<remitBill.sShopName<<"','"
		  <<pay_channel<<"','"<<remitBill.remit_fee<<"',1);";

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertSettleDB:sql_stmt:[%s].", sqlss.str().c_str());
	iRet = sql_instance.query(sqlss.str().c_str());
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertSettleDB Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		return -20;
	}
	sql_instance.free_result();

	return sql_instance.affected_rows();
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
		" DELETE FROM bill_db.%s "
		" WHERE  bm_id='%s' and pay_time >= '%s' AND pay_time <= '%s' ",
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
		" bm_id,pay_time,mch_id, transaction_id, order_no, trade_type, "
		" order_status, fee_type,total_fee,refund_id,refund_no,refund_fee,refund_status,overflow_type"
		" FROM bill_db.t_wx_overflow "
		" WHERE bm_id ='%s' and "
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
		if ((row = sql_instance.fetch_row()))
		{
			wxFlowSum.bm_id = (row[0]) ? row[0] : "";
			wxFlowSum.pay_time = (row[1]) ? row[1] : "";
			wxFlowSum.mch_id = (row[2]) ? row[2] : "";
			wxFlowSum.transaction_id = (row[3]) ? row[3] : "";
			wxFlowSum.order_no = (row[4]) ? row[4] : "";
			wxFlowSum.trade_type = (row[5]) ? row[5] : "";
			wxFlowSum.order_status = (row[6]) ? row[6] : "";
			wxFlowSum.fee_type = (row[7]) ? row[7] : "";
			wxFlowSum.total_fee = (row[8]) ? atol(row[8]) : 0;
			wxFlowSum.refund_id = (row[9]) ? row[9] : "";
			wxFlowSum.refund_no  = (row[10]) ? row[10] : "";
			wxFlowSum.refund_fee  = (row[11]) ? atol(row[11]) : 0;
			wxFlowSum.refund_status = (row[12])? row[12] : "";
			wxFlowSum.overflow_type = (row[13]) ?row[13]:"";

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

INT32 CPayTransactionFlowDao::InsertToChannelFlow(clib_mysql& sql_instance,const string& tableName,StringMap& channelMap)
{
	int iRet = 0;
	ostringstream sqlss;
    sqlss.str("");
    sqlss << "insert into "
    	  <<BILL_DB<<"."<<tableName
		  <<" (bm_id,pay_time,order_no,mch_id,channel_id,pay_channel,trade_type,order_status,"
		  <<"total_fee,refund_fee,refund_no,channel_profit_rate,channel_profit)"
		  <<" values('"<<channelMap["bm_id"]<<"','"<<channelMap["pay_time"]<<"','"<<channelMap["order_no"]<<"','"
		  <<channelMap["mch_id"]<<"','"<<channelMap["channel_id"]<<"','"<<channelMap["pay_channel"]<<"','"
		  <<channelMap["trade_type"]<<"','"<<channelMap["order_status"]<<"','"<<channelMap["total_fee"]<<"','"
		  <<channelMap["refund_fee"]<<"','"<<channelMap["refund_no"]<<"','"
		  <<channelMap["channel_profit_rate"]<<"','"<<channelMap["channel_profit"]<<"');";

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertToChannelFlow:[%s].", sqlss.str().c_str());
	iRet = sql_instance.query(sqlss.str().c_str());
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

INT32 CPayTransactionFlowDao::InsertTOOrderSuccFlow(clib_mysql& sql_instance,StringMap& orderMap)
{
	int iRet = 0;
	ostringstream sqlss;
    sqlss.str("");
    sqlss << "insert into "
    	  <<BILL_DB<<"."<<BILL_SUCC_FLOW
		  <<" (bm_id,pay_time,order_no,out_order_no,transaction_id,mch_id,channel_id,pay_channel,"
		  <<"trade_type,order_status,total_fee,total_commission,shop_amount,refund_fee,refund_no,"
		  <<"out_refund_no,refund_id,payment_profit,channel_profit,bm_profit,service_profit,"
		  <<"bill_status,settle_status)"
		  <<" values('"<<orderMap["bm_id"]<<"','"<<orderMap["pay_time"]<<"','"<<orderMap["order_no"]<<"','"
		  <<orderMap["out_order_no"]<<"','"<<orderMap["transaction_id"]<<"','"<<orderMap["mch_id"]<<"','"
		  <<orderMap["channel_id"]<<"','"<<orderMap["pay_channel"]<<"','"<<orderMap["trade_type"]<<"','"
		  <<orderMap["order_status"]<<"','"<<orderMap["total_fee"]<<"','"<<orderMap["total_commission"]<<"','"
		  <<orderMap["shop_amount"]<<"','"<<orderMap["refund_fee"]<<"','"<<orderMap["refund_no"]<<"','"
		  <<orderMap["out_refund_no"]<<"','"<<orderMap["refund_id"]<<"','"<<orderMap["payment_profit"]<<"','"
		  <<orderMap["channel_profit"]<<"','"<<orderMap["bm_profit"]<<"','"<<orderMap["service_profit"]<<"',"
		  <<"0,1);";

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertTOOrderSuccFlow:[%s].", sqlss.str().c_str());
	iRet = sql_instance.query(sqlss.str().c_str());
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

INT32 CPayTransactionFlowDao::UpdateAbnormalStatus(clib_mysql& sql_instance,StringMap& orderMap)
{
	int iRet = 0;
	ostringstream sqlss;
    sqlss.str("");
    sqlss << "update "
    	  <<BILL_DB<<"."<<BILL_ABNORMAL
		  <<" set abnormal_type = 3,porcess_status = 3,modify_time = now()"
		  <<" where bm_id = '"<<orderMap["bm_id"]<<"' and pf_order_no ='"<<orderMap["order_no"]
		  <<"' and order_status = '"<<orderMap["order_status"]<<"'";
		  if(!orderMap["out_refund_no"].empty())
		  {
			  sqlss <<" and ch_refund_no ='"<<orderMap["out_refund_no"]<<"'";
		  }

	CDEBUG_LOG(" CPayTransactionFlowDao::UpdateAbnormalStatus:[%s].", sqlss.str().c_str());
	iRet = sql_instance.query(sqlss.str().c_str());
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

INT32 CPayTransactionFlowDao::InsertwxAbnormalDB(clib_mysql& sql_instance,const std::string& strBmId,const std::string& bill_date,
							const std::string& batch_no,const std::string& pay_channel,std::vector<WxFlowSummary>& wxOverFlowVec,int index)
{
	int iRet = 0;
	ostringstream sqlss;
    sqlss.str("");
    sqlss << "insert into "
    	  <<BILL_DB<<"."<<BILL_ABNORMAL
		  <<" (bm_id,bill_date,bill_batch_no,pay_channel,cur_type,mch_id,pf_order_no,ch_trade_amount,"
		  <<"ch_refund_amount,ch_order_no,ch_refund_no,trade_date,order_status,abnormal_type,porcess_status)"
		  <<" values('"<<strBmId<<"','"<<bill_date<<"','"<<batch_no<<"','"<<pay_channel<<"','"
		  <<wxOverFlowVec[index].fee_type<<"','"<<wxOverFlowVec[index].mch_id<<"','"
		  <<wxOverFlowVec[index].order_no<<"','"
		  <<wxOverFlowVec[index].total_fee<<"','"<<wxOverFlowVec[index].refund_fee<<"','"
		  <<wxOverFlowVec[index].transaction_id<<"','"<<wxOverFlowVec[index].refund_no<<"','"
		  <<wxOverFlowVec[index].pay_time<<"','"<<wxOverFlowVec[index].order_status<<"','"
		  <<wxOverFlowVec[index].overflow_type<<"',0);";

	//CDEBUG_LOG(" CPayTransactionFlowDao::InsertwxAbnormalDB:sql_stmt:[%s].", sqlss.str().c_str());
	iRet = sql_instance.query(sqlss.str().c_str());
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

INT32 CPayTransactionFlowDao::InsertaliAbnormalDB(clib_mysql& sql_instance,const std::string& strBmId,const std::string& bill_date,const std::string& batch_no,
				const std::string& pay_channel,std::vector<AliFlowSummary>& aliOverFlowVec,int index)
{
	int iRet = 0;
	ostringstream sqlss;
    sqlss.str("");
    sqlss << "insert into "
    	  <<BILL_DB<<"."<<BILL_ABNORMAL
		  <<" (bm_id,bill_date,bill_batch_no,pay_channel,cur_type,mch_id,ch_trade_amount,"
		  <<"ch_order_no,ch_refund_no,trade_date,abnormal_type,porcess_status)"
		  <<" values('"<<strBmId<<"','"<<bill_date<<"','"<<batch_no<<"','"<<pay_channel
		  <<"','RMB','"<<aliOverFlowVec[index].mch_id<<"','"
		  <<aliOverFlowVec[index].total_fee<<"','"
		  <<aliOverFlowVec[index].transaction_id<<"','"<<aliOverFlowVec[index].refund_no<<"','"
		  <<aliOverFlowVec[index].pay_time<<"','"<<aliOverFlowVec[index].overflow_type<<"',0);";

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertwxAbnormalDB:sql_stmt:[%s].", sqlss.str().c_str());
	iRet = sql_instance.query(sqlss.str().c_str());
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
		" mch_id,transaction_id, order_no, order_status, pay_time, "
		" refund_no,total_fee "
		" FROM bill_db.t_ali_overflow "
		" WHERE bm_id ='%s' and "
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
			ali.mch_id = row[0] ? row[0] :"";
			ali.transaction_id = row[1] ? row[1] : "";
			ali.order_no = row[2] ? row[2] : "";
			ali.order_status = row[3] ? row[3] : "";
			ali.pay_time = row[4] ? row[4] : "";
			ali.refund_no = row[5] ? row[5] : "";
			ali.total_fee = row[6] ? atol(row[6]) : 0;
			CDEBUG_LOG("order_no:%s ", ali.order_no.c_str());
			ali.mch_id = strTrim(ali.mch_id);
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


INT32 CPayTransactionFlowDao::InsertSummaryDB(clib_mysql& sql_instance,const std::string& strBmId,const std::string& bill_date,const std::string& batch_no,
								const std::string& strBeginTime,const std::string& strEndTime,const char* pay_channel)
{
	int iRet = 0;
	int bill_result = 1;
	MYSQL_ROW row;
	ostringstream sqlss;
	map<string,string> summary_map;

	string order_flow_db,channel_flow_db;

	if(strcmp(pay_channel,"WXPAY") == 0)
	{
		order_flow_db = ORDER_WXPAY_FLOW;
		channel_flow_db = BILL_WXPAY_FLOW;
	}
	if(strcmp(pay_channel,"ALIPAY") == 0)
	{
		order_flow_db = ORDER_ALIPAY_FLOW;
		channel_flow_db = BILL_ALIPAY_FLOW;
	}

	//平台成功
    sqlss.str("");
    sqlss << "select bm_id,pay_channel,count(*) as pf_count,sum(total_fee) as pf_amount,sum(payment_profit) as pf_fee"
          <<" from "<<BILL_DB<<"."<<order_flow_db
		  <<" where bm_id = '"<<strBmId<<"' and pay_time >= '"<<strBeginTime
		  <<"' and pay_time <= '"<<strEndTime<<"' and order_status = 'SUCCESS'";

	CDEBUG_LOG("pf success InsertSummaryDB sql: %s", sqlss.str().c_str());
	iRet = sql_instance.query(sqlss.str().c_str());
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetOrderRefundChannelTableFlowDataSql Execute Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -20;
	}
	if ((row = sql_instance.fetch_row()))
	{
		summary_map["bm_id"] = (row[0])?row[0]:"";
		summary_map["pay_channel"] = (row[1])?row[1]:"";
		summary_map["pf_count"] = (row[2])?row[2]:"0";
		summary_map["pf_amount"] = (row[3])?row[3]:"0";
		summary_map["pf_fee"] = (row[4])?row[4]:"0";
	}
	else
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"FetchRow Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -30;
	}

	//平台退款
    sqlss.str("");
    sqlss << "select count(*) as pf_ref_count,sum(refund_fee) as pf_ref_amount ,sum(payment_profit) as pf_ref_fee"
          <<" from "<<BILL_DB<<"."<<order_flow_db
		  <<" where bm_id = '"<<strBmId<<"' and pay_time >= '"<<strBeginTime
		  <<"' and pay_time <= '"<<strEndTime<<"' and order_status = 'REFUND'";

	CDEBUG_LOG("pf refund InsertSummaryDB sql: %s", sqlss.str().c_str());
	iRet = sql_instance.query(sqlss.str().c_str());
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetOrderRefundChannelTableFlowDataSql Execute Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -20;
	}
	if ((row = sql_instance.fetch_row()))
	{
		summary_map["pf_ref_count"] = (row[0])?row[0]:"0";
		summary_map["pf_ref_amount"] = (row[1])?row[1]:"0";
		summary_map["pf_ref_fee"] = (row[2])?row[2]:"0";
	}
	else
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"FetchRow Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -30;
	}

	//渠道成功
    sqlss.str("");
    if(strcmp(pay_channel,"WXPAY") == 0)
    {
        sqlss << "select mch_id,fee_type,count(*) as ch_count,sum(total_fee *100) as ch_amount,sum(counter_fee*100) as ch_fee"
              <<" from "<<BILL_DB<<"."<<channel_flow_db
    		  <<" where bm_id = '"<<strBmId<<"' and pay_time >= '"<<strBeginTime
    		  <<"' and pay_time <= '"<<strEndTime<<"' and order_status = 'SUCCESS'";
    	CDEBUG_LOG("ch success InsertSummaryDB sql: %s", sqlss.str().c_str());
    	iRet = sql_instance.query(sqlss.str().c_str());
    	if (iRet != 0)
    	{
    		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
    			"GetOrderRefundChannelTableFlowDataSql Execute Failed.Ret[%d] Err[%u~%s]",
    			iRet, sql_instance.get_errno(), sql_instance.get_error());
    		return -20;
    	}
    	if ((row = sql_instance.fetch_row()))
    	{
    		summary_map["mch_id"] = (row[0])?row[0]:"";
    		summary_map["fee_type"] = (row[1])?row[1]:"";
    		summary_map["ch_count"] = (row[2])?row[2]:"0";
    		summary_map["ch_amount"] = (row[3])?row[3]:"0";
    		summary_map["ch_fee"] = (row[4])?row[4]:"0";
    	}
    	else
    	{
    		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
    			"FetchRow Failed.Ret[%d] Err[%u~%s]",
    			iRet, sql_instance.get_errno(), sql_instance.get_error());
    		return -30;
    	}

    	//渠道退款
        sqlss.str("");
        sqlss << "select count(*) as ch_ref_count,sum(refund_fee*100) as ch_ref_amount,sum(counter_fee*100) as ch_ref_fee"
              <<" from "<<BILL_DB<<"."<<channel_flow_db
    		  <<" where bm_id = '"<<strBmId<<"' and pay_time >= '"<<strBeginTime
    		  <<"' and pay_time <= '"<<strEndTime<<"' and order_status = 'REFUND'";

    	CDEBUG_LOG("pf refund InsertSummaryDB sql: %s", sqlss.str().c_str());
    	iRet = sql_instance.query(sqlss.str().c_str());
    	if (iRet != 0)
    	{
    		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
    			"GetOrderRefundChannelTableFlowDataSql Execute Failed.Ret[%d] Err[%u~%s]",
    			iRet, sql_instance.get_errno(), sql_instance.get_error());
    		return -20;
    	}
    	if ((row = sql_instance.fetch_row()))
    	{
    		summary_map["ch_ref_count"] = (row[0])?row[0]:"0";
    		summary_map["ch_ref_amount"] = (row[1])?row[1]:"0";
    		summary_map["ch_ref_fee"] = (row[2])?row[2]:"0";
    	}
    	else
    	{
    		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
    			"FetchRow Failed.Ret[%d] Err[%u~%s]",
    			iRet, sql_instance.get_errno(), sql_instance.get_error());
    		return -30;
    	}
    }
    else
    {
        sqlss << "select mch_id,count(*) as ch_count,sum(total_fee *100) as ch_amount,sum(service_profit*100) as ch_fee"
              <<" from "<<BILL_DB<<"."<<channel_flow_db
    		  <<" where bm_id = '"<<strBmId<<"' and pay_time >= '"<<strBeginTime
    		  <<"' and pay_time <= '"<<strEndTime<<"' and order_status = 'SUCCESS'";
    	CDEBUG_LOG("ch success InsertSummaryDB sql: %s", sqlss.str().c_str());
    	iRet = sql_instance.query(sqlss.str().c_str());
    	if (iRet != 0)
    	{
    		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
    			"GetOrderRefundChannelTableFlowDataSql Execute Failed.Ret[%d] Err[%u~%s]",
    			iRet, sql_instance.get_errno(), sql_instance.get_error());
    		return -20;
    	}
    	if ((row = sql_instance.fetch_row()))
    	{
    		summary_map["mch_id"] = (row[0])?row[0]:"";
    		//summary_map["fee_type"] = (row[1])?row[1]:"";
    		summary_map["ch_count"] = (row[1])?row[1]:"0";
    		summary_map["ch_amount"] = (row[2])?row[2]:"0";
    		summary_map["ch_fee"] = (row[3])?row[3]:"0";
    	}
    	else
    	{
    		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
    			"FetchRow Failed.Ret[%d] Err[%u~%s]",
    			iRet, sql_instance.get_errno(), sql_instance.get_error());
    		return -30;
    	}

    	//渠道退款
        sqlss.str("");
        sqlss << "select count(*) as ch_ref_count,sum(total_fee*100) as ch_ref_amount,sum(service_profit*100) as ch_ref_fee"
              <<" from "<<BILL_DB<<"."<<channel_flow_db
    		  <<" where bm_id = '"<<strBmId<<"' and pay_time >= '"<<strBeginTime
    		  <<"' and pay_time <= '"<<strEndTime<<"' and order_status = 'REFUND'";

    	CDEBUG_LOG("pf refund InsertSummaryDB sql: %s", sqlss.str().c_str());
    	iRet = sql_instance.query(sqlss.str().c_str());
    	if (iRet != 0)
    	{
    		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
    			"GetOrderRefundChannelTableFlowDataSql Execute Failed.Ret[%d] Err[%u~%s]",
    			iRet, sql_instance.get_errno(), sql_instance.get_error());
    		return -20;
    	}
    	if ((row = sql_instance.fetch_row()))
    	{
    		summary_map["ch_ref_count"] = (row[0])?row[0]:"0";
    		summary_map["ch_ref_amount"] = (row[1])?row[1]:"0";
    		summary_map["ch_ref_fee"] = (row[2])?row[2]:"0";
    	}
    	else
    	{
    		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
    			"FetchRow Failed.Ret[%d] Err[%u~%s]",
    			iRet, sql_instance.get_errno(), sql_instance.get_error());
    		return -30;
    	}
    }

	//平账成功金额
    sqlss.str("");
    sqlss << "select sum(total_fee) as normal_total_amount  "
          <<" from "<<BILL_DB<<"."<<BILL_SUCC_FLOW
		  <<" where bm_id = '"<<strBmId<<"' and pay_time >= '"<<strBeginTime<<"' and pay_time <= '"<<strEndTime
		  <<"' and pay_channel = '"<<pay_channel<<"' and order_status = 'SUCCESS'";

	CDEBUG_LOG("success  InsertSummaryDB sql: %s", sqlss.str().c_str());
	iRet = sql_instance.query(sqlss.str().c_str());
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetOrderRefundChannelTableFlowDataSql Execute Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -20;
	}
	if ((row = sql_instance.fetch_row()))
	{
		summary_map["normal_total_amount"] = (row[0])?row[0]:"0";
	}
	else
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"FetchRow Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -30;
	}

	//平账退款金额
    sqlss.str("");
    sqlss << "select sum(refund_fee) as normal_refund_amount  "
          <<" from "<<BILL_DB<<"."<<BILL_SUCC_FLOW
		  <<" where bm_id = '"<<strBmId<<"' and pay_time >= '"<<strBeginTime<<"' and pay_time <= '"<<strEndTime
		  <<"' and pay_channel = '"<<pay_channel<<"' and order_status = 'REFUND'";

	CDEBUG_LOG("refund InsertSummaryDB sql: %s", sqlss.str().c_str());
	iRet = sql_instance.query(sqlss.str().c_str());
	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetOrderRefundChannelTableFlowDataSql Execute Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -20;
	}
	if ((row = sql_instance.fetch_row()))
	{
		summary_map["normal_refund_amount"] = (row[0])?row[0]:"0";
	}
	else
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"FetchRow Failed.Ret[%d] Err[%u~%s]",
			iRet, sql_instance.get_errno(), sql_instance.get_error());
		return -30;
	}
	//计算异常
	//平台成功笔数 +平台退款笔数 — 渠道成功笔数 - 渠道退款笔数
	//平台成功  - 渠道成功   + 平台退款 - 渠道退款
	int abnormal_cnt = abs(atoi(summary_map["pf_count"].c_str()) - atoi(summary_map["ch_count"].c_str()))
					  + abs(atoi(summary_map["pf_ref_count"].c_str()) - atoi(summary_map["ch_ref_count"].c_str()));

	//平台成功金额+平台退款金额-渠道成功金额-渠道退款金额
	//平台成功 - 渠道成功   + 平台退款 - 渠道退款
	long abnormal_amount = abs(atol(summary_map["pf_amount"].c_str()) - atol(summary_map["ch_amount"].c_str()))
					  + abs(atol(summary_map["pf_ref_amount"].c_str()) - atol(summary_map["ch_ref_amount"].c_str()));

	if(abnormal_cnt != 0 || abnormal_amount != 0)
	{
		bill_result = 2;  //不符
	}

	//写汇总表
	sqlss.str("");
    sqlss << "insert into "
    	  <<BILL_DB<<"."<<BILL_SUMMARY
		  <<" (bm_id,bill_date,bill_batch_no,pay_channel,bank_inscode,cur_type,pf_total_count,"
		  <<"pf_total_amount,pf_total_refund_count,pf_total_refund_amount,pf_total_fee,pf_refund_fee,"
		  <<"ch_total_count,ch_total_amount,ch_total_refund_count,ch_total_refund_amount,"
		  <<"ch_total_fee,ch_refund_fee,abnormal_count,abnormal_amount,normal_total_amount,normal_refund_amount,bill_result)"
		  <<" values('"<<strBmId<<"','"<<bill_date<<"','"<<batch_no<<"','"<<pay_channel<<"','"
		  <<summary_map["mch_id"]<<"','"<<summary_map["fee_type"]<<"',"<<summary_map["pf_count"]<<","
		  <<summary_map["pf_amount"]<<","<<summary_map["pf_ref_count"]<<","<<summary_map["pf_ref_amount"]<<","
		  <<summary_map["pf_fee"]<<","<<summary_map["pf_ref_fee"]<<","<<summary_map["ch_count"]<<","
		  <<summary_map["ch_amount"]<<","<<summary_map["ch_ref_count"]<<","<<summary_map["ch_ref_amount"]<<","
		  <<summary_map["ch_fee"]<<","<<summary_map["ch_ref_fee"]<<","<<abnormal_cnt<<","
		  <<abnormal_amount<<",'"<<summary_map["normal_total_amount"]<<"','"<<summary_map["normal_refund_amount"]<<"',"
		  <<bill_result<<");";


	CDEBUG_LOG(" InsertSummaryDB:sql_stmt:[%s].", sqlss.str().c_str());
	iRet = sql_instance.query(sqlss.str().c_str());
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


INT32 CPayTransactionFlowDao::InsertTradeTypeOrderToDB(clib_mysql& sql_instance, const std::string& strBmId, const std::string& pay_channel, const std::string& strTableName)
{
	BEGIN_LOG(__func__);

	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into bill_db.%s (bm_id,pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit,sub_body, bill_status) select bm_id,pay_time, order_no, out_order_no, transaction_id, "
		" mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, refund_fee, refund_no, out_refund_no,  "
		" refund_id, payment_profit, channel_profit, bm_profit, service_profit,sub_body, bill_status from bill_db.t_order_all_flow "
		" where bm_id ='%s' and pay_channel = '%s'",
		strTableName.c_str(),strBmId.c_str(), pay_channel.c_str());

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
		" insert into bill_db.%s (bm_id,pay_time, order_no, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, refund_fee, refund_no, channel_profit_rate, channel_profit)"
		" select bm_id,pay_time, order_no, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, refund_fee, refund_no, channel_profit_rate, channel_profit "
		" from bill_db.t_order_channel_all_flow "
		" where bm_id ='%s' and pay_channel = '%s'",
		strTableName.c_str(), strBmId.c_str(), pay_channel.c_str());

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


INT32 CPayTransactionFlowDao::TruncateEveryPaymentTypeSysFlowData(clib_mysql& sql_instance, const std::string& strBmId,const std::string& pay_channel)
{
	BEGIN_LOG(__func__);
	Reset();
	int iRet = 0;
	std::string strOrderAllSql 				= "DELETE FROM bill_db.t_order_all_flow where bm_id ='" + strBmId + "';";
	std::string strOrderChannelAllSql 		= "DELETE FROM bill_db.t_order_channel_all_flow where bm_id ='" + strBmId + "';";

	std::string strWxpaySql 				= "DELETE FROM bill_db.t_wxpay_flow where bm_id ='" + strBmId + "';";
	std::string strWxOrderSql 				= "DELETE FROM bill_db.t_order_wxpay_flow where bm_id ='" + strBmId + "';";
	std::string strWxOrderChannelSql 		= "DELETE FROM bill_db.t_order_channel_wxpay_flow where bm_id ='" + strBmId + "';";

	std::string strAlipaySql 				= "DELETE FROM bill_db.t_alipay_flow where bm_id ='" + strBmId + "';";
	std::string strAliOrderSql 				= "DELETE FROM bill_db.t_order_alipay_flow where bm_id ='" + strBmId + "';";
	std::string strAliOrderChannelSql 		= "DELETE FROM bill_db.t_order_channel_alipay_flow where bm_id ='" + strBmId + "';";

	//CDEBUG_LOG("delete table:t_order_all_flow,t_order_channel_all_flow,t_wxpay_flow,t_order_wxpay_flow,t_order_channel_wxpay_flow,");
	//开启事务
	iRet = sql_instance.query("START TRANSACTION");
	iRet = sql_instance.query(strOrderAllSql.c_str());
	iRet = sql_instance.query(strOrderChannelAllSql.c_str());

	if(pay_channel == WX_API_PAY_CHANNEL)
	{
		iRet = sql_instance.query(strWxpaySql.c_str());
		iRet = sql_instance.query(strWxOrderSql.c_str());
		iRet = sql_instance.query(strWxOrderChannelSql.c_str());
		CDEBUG_LOG("delete table:t_order_all_flow,t_order_channel_all_flow,t_wxpay_flow,t_order_wxpay_flow,t_order_channel_wxpay_flow");
	}

	if(pay_channel == ALI_API_PAY_CHANNEL)
	{
		iRet = sql_instance.query(strAlipaySql.c_str());
		iRet = sql_instance.query(strAliOrderSql.c_str());
		iRet = sql_instance.query(strAliOrderChannelSql.c_str());
		CDEBUG_LOG("delete table:t_order_all_flow,t_order_channel_all_flow,t_alipay_flow,t_order_alipay_flow,t_order_channel_alipay_flow,");
	}
	iRet = sql_instance.query("COMMIT");//提交事务
//	CDEBUG_LOG("START TRANSACTION\n");
//	CDEBUG_LOG("wx_order_sql:[%s]\n", strWxOrderSql.c_str());
//	CDEBUG_LOG("ali_order_sql:[%s]\n", strAliOrderSql.c_str());
//	CDEBUG_LOG("wx_order_channel_sql:[%s]\n", strWxOrderChannelSql.c_str());
//	CDEBUG_LOG("ali_order_channel_sql:[%s]\n", strAliOrderChannelSql.c_str());
//	CDEBUG_LOG("COMMIT\n");
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

INT32 CPayTransactionFlowDao::LoadFiletoDB(clib_mysql& sql_instance, const std::string& strFileName,const std::string& strTableName)
{
	Reset();
	int iRet = 0;
	std::string strAbsTableName(BILL_DB);
			strAbsTableName    += ".";
			strAbsTableName    += strTableName;
	std::string strLoadFileSql 	= "LOAD DATA LOCAL INFILE \"" + strFileName + "\" INTO TABLE " + strAbsTableName +" CHARACTER SET utf8 FIELDS TERMINATED BY ',';";
	CDEBUG_LOG("load file sql:[%s]\n", strLoadFileSql.c_str());

	iRet = sql_instance.query(strLoadFileSql.c_str());

	if (iRet != 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"TruncateEveryPaymentTypeSysFlowData Execute"
			"Failed.Ret[%d] Err[%u~%s]",
			iRet,
			sql_instance.get_errno(),
			sql_instance.get_error());
		CDEBUG_LOG("errMsg [%s]",m_szErrMsg);
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
		" insert into bill_db.t_bill_success_flow (bm_id,pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit,sub_body, bill_status) select shop.bm_id,shop.pay_time, shop.order_no, shop.out_order_no, shop.transaction_id, "
		" shop.mch_id, shop.channel_id, shop.pay_channel, shop.trade_type, shop.order_status, shop.total_fee, shop.total_commission, shop.shop_amount, shop.refund_fee, shop.refund_no, shop.out_refund_no,  "
		" shop.refund_id, shop.payment_profit, shop.channel_profit, shop.bm_profit, shop.service_profit, shop.sub_body,shop.bill_status from bill_db.t_alipay_flow as ali inner JOIN  bill_db.t_order_alipay_flow as shop "
		" on shop.bm_id = ali.bm_id and shop.order_no = ali.order_no  and ABS(ali.total_fee * 100) = shop.total_fee  and shop.order_status = ali.order_status "
		" where shop.bm_id ='%s' and shop.pay_time >= '%s' and shop.pay_time <= '%s' and shop.order_status = '%s'",  //SUCCESS
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str(), order_status.c_str());

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
		" insert into bill_db.t_bill_success_flow (bm_id,pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit,sub_body, bill_status) select shop.bm_id,shop.pay_time, shop.order_no, shop.out_order_no, shop.transaction_id, "
		" shop.mch_id, shop.channel_id, shop.pay_channel, shop.trade_type, shop.order_status, shop.total_fee, shop.total_commission, shop.shop_amount, shop.refund_fee, shop.refund_no, shop.out_refund_no,  "
		" shop.refund_id, shop.payment_profit, shop.channel_profit, shop.bm_profit, shop.service_profit,shop.sub_body, shop.bill_status from bill_db.t_alipay_flow as ali inner JOIN  bill_db.t_order_alipay_flow as shop "
		" on shop.bm_id = ali.bm_id and shop.order_no = ali.order_no  and ali.refund_no = shop.refund_no and ABS(ali.total_fee * 100) = shop.refund_fee and shop.order_status = ali.order_status "
		" where shop.bm_id ='%s' and shop.pay_time >= '%s' and shop.pay_time <= '%s' and shop.order_status = 'REFUND'",  //SUCCESS
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

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
		" insert into bill_db.t_ali_overflow (bm_id,transaction_id, order_no, order_status, goods_name, create_time, pay_time, store_no, store_name, operator_name, terminal_no, each_account, total_fee, shop_net_receipts, alipay_red_fee, score_amount, alipay_discount_amount, "
		" shop_discount_amount, coupon_write_off_fee, coupon_name, shop_red_fee, card_consume_fee, refund_no, service_profit, net_paid_in, mch_id, trade_mode, remark) SELECT ali.bm_id,ali.transaction_id, ali.order_no, ali.order_status, ali.goods_name, ali.create_time, ali.pay_time, "
		" ali.store_no, ali.store_name, ali.operator_name, ali.terminal_no, ali.each_account, ali.total_fee, ali.shop_net_receipts, ali.alipay_red_fee, ali.score_amount, ali.alipay_discount_amount,  ali.shop_discount_amount, ali.coupon_write_off_fee, ali.coupon_name,  "
		" ali.shop_red_fee, ali.card_consume_fee, ali.refund_no, ali.service_profit, ali.net_paid_in, ali.mch_id, ali.trade_mode, ali.remark FROM bill_db.t_alipay_flow AS ali LEFT JOIN  bill_db.t_order_alipay_flow AS shop ON shop.bm_id = ali.bm_id and shop.order_no = ali.order_no "
		" AND ABS(ali.total_fee * 100) = shop.total_fee  AND shop.order_status = ali.order_status WHERE ali.bm_id ='%s' and ali.pay_time >= '%s' AND ali.pay_time <= '%s' AND ali.order_status = '%s' AND shop.order_no IS NULL",//SUCCESS
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str(), order_status.c_str());

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
		" insert into bill_db.t_ali_overflow (bm_id,transaction_id, order_no, order_status, goods_name, create_time, pay_time, store_no, store_name, operator_name, terminal_no, each_account, total_fee, shop_net_receipts, alipay_red_fee, score_amount, alipay_discount_amount, "
		" shop_discount_amount, coupon_write_off_fee, coupon_name, shop_red_fee, card_consume_fee, refund_no, service_profit, net_paid_in, mch_id, trade_mode, remark) SELECT ali.bm_id,ali.transaction_id, ali.order_no, ali.order_status, ali.goods_name, ali.create_time, ali.pay_time, "
		" ali.store_no, ali.store_name, ali.operator_name, ali.terminal_no, ali.each_account, ali.total_fee, ali.shop_net_receipts, ali.alipay_red_fee, ali.score_amount, ali.alipay_discount_amount,  ali.shop_discount_amount, ali.coupon_write_off_fee, ali.coupon_name,  "
		" ali.shop_red_fee, ali.card_consume_fee, ali.refund_no, ali.service_profit, ali.net_paid_in, ali.mch_id, ali.trade_mode, ali.remark FROM bill_db.t_alipay_flow AS ali LEFT JOIN  bill_db.t_order_alipay_flow AS shop ON shop.bm_id = ali.bm_id and shop.order_no = ali.order_no "
		" AND ABS(ali.total_fee * 100) = shop.refund_fee  and ali.refund_no = shop.refund_no AND shop.order_status = ali.order_status WHERE ali.bm_id ='%s' and ali.pay_time >= '%s' AND ali.pay_time <= '%s' AND ali.order_status = 'REFUND' AND shop.order_no IS NULL",//SUCCESS
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

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
	snprintf(sql_succ_stmt, sizeof(sql_succ_stmt), "UPDATE bill_db.t_alipay_flow set order_status = 'SUCCESS' where bm_id ='%s' and order_status = '交易'", strBmId.c_str());
	
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
	snprintf(sql_refund_stmt, sizeof(sql_refund_stmt), "UPDATE bill_db.t_alipay_flow set order_status = 'REFUND' where bm_id ='%s' and order_status = '退款'", strBmId.c_str());

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
