/*
 * CPayTransSybaseDao.cpp
 *
 *  Created on: 2017年5月27日
 *      Author: hawrkchen
 */

#include "CPayTransSybaseDao.h"
#include "log/clog.h"


CPayTransSybaseDao::CPayTransSybaseDao()
{
    // TODO;
}

CPayTransSybaseDao::~CPayTransSybaseDao()
{
    // TODO;
}

INT32 CPayTransSybaseDao::InsertTradeTypeOrderToDB(CSyBase& sql_instance, const std::string& strBmId, const std::string& pay_channel, const std::string& strTableName)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into %s_%s (pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit, bill_status) select pay_time, order_no, out_order_no, transaction_id, "
		" mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, refund_fee, refund_no, out_refund_no,  "
		" refund_id, payment_profit, channel_profit, bm_profit, service_profit, bill_status from t_order_all_flow_%s "
		" where pay_channel = '%s'",
		strTableName.c_str(), strBmId.c_str(),strBmId.c_str(), pay_channel.c_str());

	CDEBUG_LOG(" CPayTransSybaseDao::InsertTradeTypeOrderToDB:sql_stmt:[%s].", sql_stmt);
	//iRet = sql_instance.query(sql_stmt);
	int _count = 0;
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	iRet = spp_bill_Insert_Update_Del(0,&param,sql_stmt,&_count);
	CDEBUG_LOG("CPayTransSybaseDao::InsertTradeTypeOrderToDB end ,iRet[%d]",iRet);
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertTradeTypeOrderToDB Execute"
			"Failed.Ret[%d] ",
			iRet);
		return -20;
	}
	//sql_instance.free_result();

	return iRet;

	return 0;
}

INT32 CPayTransSybaseDao::InsertTradeTypeOrderChannelToDB(CSyBase& sql_instance, const std::string& strBmId, const std::string& pay_channel, const std::string& strTableName)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into %s_%s (pay_time, order_no, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, refund_fee, refund_no, channel_profit_rate, channel_profit)"
		" select pay_time, order_no, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, refund_fee, refund_no, channel_profit_rate, channel_profit "
		" from t_order_channel_all_flow_%s "
		" where pay_channel = '%s'",
		strTableName.c_str(), strBmId.c_str(),strBmId.c_str(), pay_channel.c_str());

	CDEBUG_LOG(" CPayTransSybaseDao::InsertTradeTypeOrderChannelToDB:sql_stmt:[%s].", sql_stmt);
	//iRet = sql_instance.query(sql_stmt);
	int _count = 0;
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	iRet = spp_bill_Insert_Update_Del(0,&param,sql_stmt,&_count);
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertTradeTypeOrderChannelToDB Execute"
			"Failed.Ret[%d]",
			iRet);
		return -20;
	}
	//sql_instance.free_result();
	return iRet;
}

INT32 CPayTransSybaseDao::RemoveAlipayDifferenceSuccState(CSyBase& sql_instance, const std::string& strBmId)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	Reset();
	int iRet = 0;
	char sql_succ_stmt[1024];
	snprintf(sql_succ_stmt, sizeof(sql_succ_stmt), "UPDATE t_alipay_flow_%s set order_status = 'SUCCESS' where order_status = '交易'", strBmId.c_str());

	//开启事务
	//iRet = sql_instance.query(sql_succ_stmt);
	int _count = 0;
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	iRet = spp_bill_Insert_Update_Del(0,&param,sql_succ_stmt,&_count);
	CDEBUG_LOG("sql_succ_stmt:[%s]\n", sql_succ_stmt);
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"RemoveAlipayDifferenceSuccState Execute"
			"Failed.Ret[%d]",
			iRet);
		return -20;
	}
	//sql_instance.free_result();

	return iRet;
}

INT32 CPayTransSybaseDao::RemoveAlipayDifferenceRefundState(CSyBase& sql_instance, const std::string& strBmId)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	Reset();
	int iRet = 0;
	char sql_refund_stmt[1024];
	snprintf(sql_refund_stmt, sizeof(sql_refund_stmt), "UPDATE t_alipay_flow_%s set order_status = 'REFUND' where order_status = '退款'", strBmId.c_str());

	//iRet = sql_instance.query(sql_refund_stmt);
	int _count = 0;
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	iRet = spp_bill_Insert_Update_Del(0,&param,sql_refund_stmt,&_count);
	CDEBUG_LOG("sql_refund_stmt:[%s]\n", sql_refund_stmt);
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"RemoveAlipayDifferenceRefundState Execute"
			"Failed.Ret[%d]",
			iRet);
		return -20;
	}
	//sql_instance.free_result();

	return iRet;
}

INT32 CPayTransSybaseDao::EmptyTableData(CSyBase& sql_instance,
									const std::string& TableName,const std::string& strBmid,
									 const std::string& strBeginTime,const std::string& strEndTime)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	int iRet = 0;

	Reset();

	char sql_stmt[4096];

	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" DELETE FROM %s_%s "
		" WHERE pay_time >= '%s' AND pay_time <= '%s' ",
		TableName.c_str(),strBmid.c_str(),strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG(" CPayTransSybaseDao::EmptyTableData:sql_stmt:[%s].", sql_stmt);
	//iRet = sql_instance.query(sql_stmt);
	int _count = 0;
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	iRet = spp_bill_Insert_Update_Del(0,&param,sql_stmt,&_count);
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"EmptyBillSucFlowData Execute"
			"Failed.Ret[%d]",
			iRet);
		return -20;
	}
	//sql_instance.free_result();

	return iRet;
}

/*
 * @brief 微信对账操作
 */

INT32 CPayTransSybaseDao::InsertPayIdenticalWxToDB(CSyBase& sql_instance, const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	int iRet;
	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into t_bill_success_flow_%s (pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit, bill_status) select shop.pay_time, shop.order_no, shop.out_order_no, shop.transaction_id, "
		" shop.mch_id, shop.channel_id, shop.pay_channel, shop.trade_type, shop.order_status, shop.total_fee, shop.total_commission, shop.shop_amount, shop.refund_fee, shop.refund_no, shop.out_refund_no,  "
		" shop.refund_id, shop.payment_profit, shop.channel_profit, shop.bm_profit, shop.service_profit, shop.bill_status from t_wxpay_flow_%s as wx inner JOIN  t_order_wxpay_flow_%s as shop "
		" on shop.order_no = wx.order_no  and (wx.total_fee * 100) = shop.total_fee and shop.order_status = wx.order_status "
		" where shop.pay_time >= '%s' and shop.pay_time <= '%s' and shop.order_status = 'SUCCESS' ",
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertPayIdenticalWxToDB:sql_stmt:[%s].", sql_stmt);
	//iRet = sql_instance.query(sql_stmt);

	int _count = 0;
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	iRet = spp_bill_Insert_Update_Del(0,&param,sql_stmt,&_count);

	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertPayIdenticalWxToDB Execute"
			"Failed.Ret[%d]",
			iRet);
		return -20;
	}
	//sql_instance.free_result();

	return iRet;

}

INT32 CPayTransSybaseDao::InsertRefundIdenticalWxToDB(CSyBase& sql_instance, const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into t_bill_success_flow_%s (pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit, bill_status) select shop.pay_time, shop.order_no, shop.out_order_no, shop.transaction_id, "
		" shop.mch_id, shop.channel_id, shop.pay_channel, shop.trade_type, shop.order_status, shop.total_fee, shop.total_commission, shop.shop_amount, shop.refund_fee, shop.refund_no, shop.out_refund_no, "
		" shop.refund_id, shop.payment_profit, shop.channel_profit, shop.bm_profit, shop.service_profit, shop.bill_status from t_wxpay_flow_%s as wx inner JOIN  t_order_wxpay_flow_%s as shop "
		" on shop.order_no = wx.order_no and (wx.refund_fee * 100) = shop.refund_fee and shop.order_status = wx.order_status "
		" where shop.pay_time >= '%s' and shop.pay_time <= '%s' and shop.order_status = 'REFUND' ",
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertRefundIdenticalToDB:sql_stmt:[%s].", sql_stmt);
	//iRet = sql_instance.query(sql_stmt);
	int _count = 0;
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	iRet = spp_bill_Insert_Update_Del(0,&param,sql_stmt,&_count);
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertRefundIdenticalToDB Execute"
			"Failed.Ret[%d] ",
			iRet);
		return -20;
	}
	return iRet;
}

INT32 CPayTransSybaseDao::InsertPayDistinctWxToDB(CSyBase& sql_instance, const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);

	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into t_wx_overflow_%s (pay_time, app_id, mch_id, sub_mch_id, device_info, transaction_id, order_no, openid, trade_type, order_status, bank_type, fee_type, total_fee, red_amount, refund_id, refund_no, "
		" refund_fee, red_refund_amount, refund_type, refund_status, goods_name, shop_packet, counter_fee, rate) SELECT wx.pay_time, wx.app_id, wx.mch_id, wx.sub_mch_id, wx.device_info, wx.transaction_id, wx.order_no,"
		" wx.openid, wx.trade_type, wx.order_status, wx.bank_type, wx.fee_type, wx.total_fee, wx.red_amount, wx.refund_id, wx.refund_no,  wx.refund_fee, wx.red_refund_amount, wx.refund_type, wx.refund_status, wx.goods_name, "
		" wx.shop_packet, wx.counter_fee, wx.rate FROM t_wxpay_flow_%s AS wx LEFT JOIN  t_order_wxpay_flow_%s AS shop ON shop.order_no = wx.order_no AND (wx.total_fee * 100) = shop.total_fee  AND shop.order_status = wx.order_status "
		" WHERE wx.pay_time >= '%s' AND wx.pay_time <= '%s' AND wx.order_status = 'SUCCESS' AND shop.order_no IS NULL",
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertPayDistinctWxToDB:sql_stmt:[%s].", sql_stmt);
	//iRet = sql_instance.query(sql_stmt);
	int _count = 0;
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	iRet = spp_bill_Insert_Update_Del(0,&param,sql_stmt,&_count);
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertPayDistinctWxToDB Execute"
			"Failed.Ret[%d] ",
			iRet);
		return -20;
	}
	return -iRet;

}

INT32 CPayTransSybaseDao::InsertRefundDistinctWxToDB(CSyBase& sql_instance, const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into t_wx_overflow_%s (pay_time, app_id, mch_id, sub_mch_id, device_info, transaction_id, order_no, openid, trade_type, order_status, bank_type, fee_type, total_fee, red_amount, refund_id, refund_no, "
		" refund_fee, red_refund_amount, refund_type, refund_status, goods_name, shop_packet, counter_fee, rate) SELECT wx.pay_time, wx.app_id, wx.mch_id, wx.sub_mch_id, wx.device_info, wx.transaction_id, wx.order_no,"
		" wx.openid, wx.trade_type, wx.order_status, wx.bank_type, wx.fee_type, wx.total_fee, wx.red_amount, wx.refund_id, wx.refund_no,  wx.refund_fee, wx.red_refund_amount, wx.refund_type, wx.refund_status, wx.goods_name, "
		" wx.shop_packet, wx.counter_fee, wx.rate FROM t_wxpay_flow_%s AS wx LEFT JOIN  t_order_wxpay_flow_%s AS shop ON shop.order_no = wx.order_no AND (wx.refund_fee * 100) = shop.refund_fee  AND shop.order_status = wx.order_status"
		" WHERE wx.pay_time >= '%s' AND wx.pay_time <= '%s' AND wx.order_status = 'REFUND' AND shop.order_no IS NULL",
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertRefundDistinctWxToDB:sql_stmt:[%s].", sql_stmt);
	//iRet = sql_instance.query(sql_stmt);
	int _count = 0;
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	iRet = spp_bill_Insert_Update_Del(0,&param,sql_stmt,&_count);
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertRefundDistinctWxToDB Execute"
			"Failed.Ret[%d]",
			iRet);
		return -20;
	}
	//sql_instance.free_result();

	return iRet;
}

/*
 * @brief 支付宝对账操作
 *
 */
INT32 CPayTransSybaseDao::InsertAliPayIdenticalToDB(CSyBase& sql_instance,
									const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime, const std::string& order_status)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into t_bill_success_flow_%s (pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit, bill_status) select shop.pay_time, shop.order_no, shop.out_order_no, shop.transaction_id, "
		" shop.mch_id, shop.channel_id, shop.pay_channel, shop.trade_type, shop.order_status, shop.total_fee, shop.total_commission, shop.shop_amount, shop.refund_fee, shop.refund_no, shop.out_refund_no,  "
		" shop.refund_id, shop.payment_profit, shop.channel_profit, shop.bm_profit, shop.service_profit, shop.bill_status from t_alipay_flow_%s as ali inner JOIN  t_order_alipay_flow_%s as shop "
		" on shop.order_no = ali.order_no  and ABS(ali.total_fee * 100) = shop.total_fee  and shop.order_status = ali.order_status "
		" where shop.pay_time >= '%s' and shop.pay_time <= '%s' and shop.order_status = '%s'",  //SUCCESS
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str(), order_status.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertAliPayIdenticalToDB:sql_stmt:[%s].", sql_stmt);
	//iRet = sql_instance.query(sql_stmt);
	int _count = 0;
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	iRet = spp_bill_Insert_Update_Del(0,&param,sql_stmt,&_count);
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertAliPayIdenticalToDB Execute"
			"Failed.Ret[%d]",
			iRet);
		return -20;
	}
	//sql_instance.free_result();

	return iRet;
}

INT32 CPayTransSybaseDao::InsertAliPayIdenticalRefundToDB(CSyBase& sql_instance,
	const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into t_bill_success_flow_%s (pay_time, order_no, out_order_no, transaction_id, mch_id, channel_id, pay_channel, trade_type, order_status, total_fee, total_commission, shop_amount, "
		" refund_fee, refund_no, out_refund_no, refund_id, payment_profit, channel_profit, bm_profit, service_profit, bill_status) select shop.pay_time, shop.order_no, shop.out_order_no, shop.transaction_id, "
		" shop.mch_id, shop.channel_id, shop.pay_channel, shop.trade_type, shop.order_status, shop.total_fee, shop.total_commission, shop.shop_amount, shop.refund_fee, shop.refund_no, shop.out_refund_no,  "
		" shop.refund_id, shop.payment_profit, shop.channel_profit, shop.bm_profit, shop.service_profit, shop.bill_status from t_alipay_flow_%s as ali inner JOIN  t_order_alipay_flow_%s as shop "
		" on shop.order_no = ali.order_no  and ali.refund_no = shop.refund_no and ABS(ali.total_fee * 100) = shop.refund_fee and shop.order_status = ali.order_status "
		" where shop.pay_time >= '%s' and shop.pay_time <= '%s' and shop.order_status = 'REFUND'",  //SUCCESS
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertAliPayIdenticalRefundToDB:sql_stmt:[%s].", sql_stmt);
	//iRet = sql_instance.query(sql_stmt);
	int _count = 0;
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	iRet = spp_bill_Insert_Update_Del(0,&param,sql_stmt,&_count);
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertAliPayIdenticalToDB Execute"
			"Failed.Ret[%d]",
			iRet);
		return -20;
	}
	//sql_instance.free_result();

	return iRet;
}

INT32 CPayTransSybaseDao::InsertAliPayDistinctToDB(CSyBase& sql_instance, const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime, const std::string& order_status)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into t_ali_overflow_%s (transaction_id, order_no, order_status, goods_name, create_time, pay_time, store_no, store_name, operator_name, terminal_no, each_account, total_fee, shop_net_receipts, alipay_red_fee, score_amount, alipay_discount_amount, "
		" shop_discount_amount, coupon_write_off_fee, coupon_name, shop_red_fee, card_consume_fee, refund_no, service_profit, net_paid_in, mch_id, trade_mode, remark) SELECT ali.transaction_id, ali.order_no, ali.order_status, ali.goods_name, ali.create_time, ali.pay_time, "
		" ali.store_no, ali.store_name, ali.operator_name, ali.terminal_no, ali.each_account, ali.total_fee, ali.shop_net_receipts, ali.alipay_red_fee, ali.score_amount, ali.alipay_discount_amount,  ali.shop_discount_amount, ali.coupon_write_off_fee, ali.coupon_name,  "
		" ali.shop_red_fee, ali.card_consume_fee, ali.refund_no, ali.service_profit, ali.net_paid_in, ali.mch_id, ali.trade_mode, ali.remark FROM t_alipay_flow_%s AS ali LEFT JOIN  t_order_alipay_flow_%s AS shop ON shop.order_no = ali.order_no "
		" AND ABS(ali.total_fee * 100) = shop.total_fee  AND shop.order_status = ali.order_status WHERE ali.pay_time >= '%s' AND ali.pay_time <= '%s' AND ali.order_status = '%s' AND shop.order_no IS NULL",//SUCCESS
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str(), order_status.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertAliPayDistinctToDB:sql_stmt:[%s].", sql_stmt);
	//iRet = sql_instance.query(sql_stmt);
	int _count = 0;
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	iRet = spp_bill_Insert_Update_Del(0,&param,sql_stmt,&_count);
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertAliPayDistinctToDB Execute"
			"Failed.Ret[%d] ",
			iRet);
		return -20;
	}
	//sql_instance.free_result();

	return iRet;
}

INT32 CPayTransSybaseDao::InsertAliPayDistinctRefundToDB(CSyBase& sql_instance, const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	int iRet = 0;

	Reset();

	char sql_stmt[4096];
	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" insert into t_ali_overflow_%s (transaction_id, order_no, order_status, goods_name, create_time, pay_time, store_no, store_name, operator_name, terminal_no, each_account, total_fee, shop_net_receipts, alipay_red_fee, score_amount, alipay_discount_amount, "
		" shop_discount_amount, coupon_write_off_fee, coupon_name, shop_red_fee, card_consume_fee, refund_no, service_profit, net_paid_in, mch_id, trade_mode, remark) SELECT ali.transaction_id, ali.order_no, ali.order_status, ali.goods_name, ali.create_time, ali.pay_time, "
		" ali.store_no, ali.store_name, ali.operator_name, ali.terminal_no, ali.each_account, ali.total_fee, ali.shop_net_receipts, ali.alipay_red_fee, ali.score_amount, ali.alipay_discount_amount,  ali.shop_discount_amount, ali.coupon_write_off_fee, ali.coupon_name,  "
		" ali.shop_red_fee, ali.card_consume_fee, ali.refund_no, ali.service_profit, ali.net_paid_in, ali.mch_id, ali.trade_mode, ali.remark FROM t_alipay_flow_%s AS ali LEFT JOIN  t_order_alipay_flow_%s AS shop ON shop.order_no = ali.order_no "
		" AND ABS(ali.total_fee * 100) = shop.refund_fee  and ali.refund_no = shop.refund_no AND shop.order_status = ali.order_status WHERE ali.pay_time >= '%s' AND ali.pay_time <= '%s' AND ali.order_status = 'REFUND' AND shop.order_no IS NULL",//SUCCESS
		strBmId.c_str(), strBmId.c_str(), strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG(" CPayTransactionFlowDao::InsertAliPayDistinctRefundToDB:sql_stmt:[%s].", sql_stmt);
	//iRet = sql_instance.query(sql_stmt);
	int _count = 0;
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	iRet = spp_bill_Insert_Update_Del(0,&param,sql_stmt,&_count);
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"InsertAliPayDistinctRefundToDB Execute"
			"Failed.Ret[%d]",
			iRet);
		return -20;
	}
	//sql_instance.free_result();

	return iRet;
}

/*
 * @brief 溢出表操作
 */
INT32 CPayTransSybaseDao::GetWxOverFlowData(CSyBase& sql_instance, const std::string& strBmId, const std::string& strBeginTime,
							const std::string& strEndTime, std::vector<WxFlowSummary>& wxOverFlowVec)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);

	int iRet = 0;
	//MYSQL_ROW row;
	int row_count = 0;
	Reset();

	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	char sql_stmt[4096];
	char sql_count[512];
	snprintf(sql_count,
		sizeof(sql_count),
		" SELECT count(*)"
		" FROM t_wx_overflow_%s "
		" WHERE "
		" pay_time >= '%s'  AND  pay_time <= '%s'",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	//CDEBUG_LOG("GetWxOverFlowData count_sql: %s", sql_count);
	iRet = spp_bill_getrecord_count(0,&param,sql_count,&row_count);

	CDEBUG_LOG("GetWxOverFlowData row_count = [%d]",row_count);
	if(row_count <= 0)
	{
		return RET_HASNOREC;
	}

	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" SELECT "
		" convert(datetime,convert(char(8),pay_time,112)||' '||convert(char(8),pay_time,108)) as pay_time,"
		" transaction_id, order_no, trade_type, "
		" order_status, refund_id, refund_no "
		" FROM t_wx_overflow_%s "
		" WHERE "
		" pay_time >= '%s'  AND  pay_time <= '%s'",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	//转了pay_time格式也没个卵用
	CDEBUG_LOG("GetWxOverFlowData sql: %s", sql_stmt);

	stGetWXOverFlowData_Resp pGetWXOver[row_count];

	iRet = spp_bill_GetWXOverFlowData(0,&param,pGetWXOver,sql_stmt,row_count);
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetWxOverFlowData Execute Failed.Ret[%d]",
			iRet);
		return -20;
	}

	for (int i = 0; i < row_count; ++i)
	{
		WxFlowSummary wxFlowSum;
		wxFlowSum.Reset();

		wxFlowSum.pay_time = pGetWXOver[i].pay_time;//(row[0]) ? row[0] : "";
		wxFlowSum.transaction_id = pGetWXOver[i].transaction_id;//(row[1]) ? row[1] : "";
		wxFlowSum.order_no = pGetWXOver[i].order_no;//(row[2]) ? row[2] : "";
		wxFlowSum.trade_type = pGetWXOver[i].trade_type;//(row[3]) ? row[3] : "";
		wxFlowSum.order_status = pGetWXOver[i].order_status;//(row[4]) ? row[4] : "";
		wxFlowSum.refund_id = pGetWXOver[i].refund_id;//(row[5]) ? row[5] : "";
		wxFlowSum.refund_no  = pGetWXOver[i].refund_no;//(row[6]) ? row[6] : "";

		//
		rtrim(wxFlowSum.order_no);
		rtrim(wxFlowSum.transaction_id);
		rtrim(wxFlowSum.order_status);
		rtrim(wxFlowSum.refund_id);
		rtrim(wxFlowSum.refund_no);

		wxOverFlowVec.push_back(wxFlowSum);
	}
	return RET_HASREC;
}

INT32 CPayTransSybaseDao::GetAliOverFlowData(CSyBase& sql_instance,
						const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
						std::vector<AliFlowSummary>& aliOverFlowList)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	int iRet = 0;
	int row_count = 0;
	//MYSQL_ROW row;
	Reset();
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	char sql_stmt[4096];
	char sql_count[512];
	snprintf(sql_count,
		sizeof(sql_count),
		" SELECT count(*)"
		" FROM t_ali_overflow_%s "
		" WHERE "
		" pay_time >= '%s'  AND  pay_time <= '%s'",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	iRet = spp_bill_getrecord_count(0,&param,sql_count,&row_count);

	CDEBUG_LOG("GetAliOverFlowData row_count = [%d]",row_count);
	if(row_count <= 0)
	{
		return RET_HASNOREC;
	}

	snprintf(sql_stmt,
		sizeof(sql_stmt),
		" SELECT "
		" transaction_id, order_no, order_status, pay_time, "
		" refund_no "
		" FROM t_ali_overflow_%s "
		" WHERE "
		" pay_time >= '%s'  AND  pay_time <= '%s'",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG("GetAliOverFlowData sql: %s", sql_stmt);

	stGetAliOverFlowData_Resp pGetAliOver[row_count];

	iRet = spp_bill_GetAliOverFlowData(0,&param,pGetAliOver,sql_stmt,row_count);

	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetAliOverFlowData Execute Failed.Ret[%d]",
			iRet);
		return -20;
	}

	for (int i = 0; i < row_count; ++i)
	{
		AliFlowSummary ali;
		//if ((row = sql_instance.fetch_row()))
		//{
			//CDEBUG_LOG("row[0]:%s row[1]:%s row[2]:%s row[3]:%s row[4]:%s", row[0], row[1], row[2], row[3], row[4]);
		ali.transaction_id = pGetAliOver[i].transaction_id;//row[0] ? row[0] : "";
		ali.order_no = pGetAliOver[i].order_no;//row[1] ? row[1] : "";
		ali.order_status = pGetAliOver[i].order_status;//row[2] ? row[2] : "";
		ali.pay_time = pGetAliOver[i].pay_time;//row[3] ? row[3] : "";
		ali.refund_no = pGetAliOver[i].refund_no;//row[4] ? row[4] : "";
//		ali.transaction_id = strTrim(ali.transaction_id);
//		ali.order_no = strTrim(ali.order_no);
//		ali.order_status = strTrim(ali.order_status);
//		ali.pay_time = strTrim(ali.pay_time);
//		ali.refund_no = strTrim(ali.refund_no);

		rtrim(ali.transaction_id);
		rtrim(ali.order_no);
		rtrim(ali.order_status);
		rtrim(ali.refund_no);

		aliOverFlowList.push_back(ali);

	}

	return RET_HASREC;
}

/*
 * @brief
 */
INT32 CPayTransSybaseDao::TruncateEveryPaymentTypeSysFlowData(CSyBase& sql_instance, const std::string& strBmId,const std::string& channel)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);

	Reset();
	int iRet = 0;

	char strAllFlowSql[128] = {0};
	char strAllChannelSql[128] = {0};

	char strchannelFlowSql[128] = {0};
	char strchannelOrderSql[128] = {0};
	char strOrderChannelSql[128] = {0};

	sprintf(strAllFlowSql,"%s%s","TRUNCATE TABLE t_order_all_flow_",strBmId.c_str());
	sprintf(strAllChannelSql,"%s%s","TRUNCATE TABLE t_order_channel_all_flow_",strBmId.c_str());

	if(channel == "wx")
	{
		sprintf(strchannelFlowSql,"%s%s","TRUNCATE TABLE t_wxpay_flow_",strBmId.c_str());
		sprintf(strchannelOrderSql,"%s%s","TRUNCATE TABLE t_order_wxpay_flow_",strBmId.c_str());
		sprintf(strOrderChannelSql,"%s%s","TRUNCATE TABLE t_order_channel_wxpay_flow_",strBmId.c_str());
	}
	else if(channel == "ali")
	{
		sprintf(strchannelFlowSql,"%s%s","TRUNCATE TABLE t_alipay_flow_",strBmId.c_str());
		sprintf(strchannelOrderSql,"%s%s","TRUNCATE TABLE t_order_alipay_flow_",strBmId.c_str());
		sprintf(strOrderChannelSql,"%s%s","TRUNCATE TABLE t_order_channel_alipay_flow_",strBmId.c_str());
	}

	int _count = 0;
	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	iRet = spp_bill_Insert_Update_Del(0,&param,strAllFlowSql,&_count);
	iRet = spp_bill_Insert_Update_Del(0,&param,strAllChannelSql,&_count);
	iRet = spp_bill_Insert_Update_Del(0,&param,strchannelFlowSql,&_count);
	iRet = spp_bill_Insert_Update_Del(0,&param,strchannelOrderSql,&_count);
	iRet = spp_bill_Insert_Update_Del(0,&param,strOrderChannelSql,&_count);

	//iRet = spp_bill_Transaction(0,&param,strWxOrderSql,strAliOrderSql,strWxOrderChannelSql,strAliOrderChannelSql,&_count);

//	CDEBUG_LOG("START TRANSACTION\n");
//	CDEBUG_LOG("wx_order_sql:[%s]\n", strWxOrderSql);
//	CDEBUG_LOG("ali_order_sql:[%s]\n", strAliOrderSql);
//	CDEBUG_LOG("wx_order_channel_sql:[%s]\n", strWxOrderChannelSql);
//	CDEBUG_LOG("ali_order_channel_sql:[%s]\n", strAliOrderChannelSql);
//	CDEBUG_LOG("COMMIT\n");
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"TruncateEveryPaymentTypeSysFlowData Execute"
			"Failed.Ret[%d]",
			iRet);
		return -20;
	}
	//sql_instance.free_result();

	return iRet;
}

/*
 *
 *
 * Settlement  结算操作
 *
 *
 *
 */

int CPayTransSybaseDao::GetPayBillData(CSyBase& sql_instance,
									const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
									std::map<std::string, OrderPayBillSumary>& orderPayBillSMap)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	int iRet = 0;
	int row_count = 0;
	//MYSQL_ROW row;

	Reset();

	char sql_stmt[4096];
	char sql_count[512];

	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	snprintf(sql_count, sizeof(sql_count),
		" SELECT COUNT(DISTINCT(mch_id)) from t_bill_success_flow_%s where order_status = 'SUCCESS' AND "
		" pay_time >= '%s'  AND  pay_time <= '%s' GROUP BY mch_id ",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	iRet = spp_bill_getrecord_count(0,&param,sql_count,&row_count);

	CDEBUG_LOG(" row_count = [%d]",row_count);
	if(row_count <= 0)
	{
		return RET_HASNOREC;
	}

	snprintf(sql_stmt, sizeof(sql_stmt),
		" SELECT mch_id, SUM(shop_amount) as shop_amount, SUM(channel_profit) as channel_profit,"
		" SUM(service_profit) as service_profit, SUM(payment_profit) as payment_profit, "
		"SUM(bm_profit) as bm_profit, SUM(total_commission) as total_commission,"
		" SUM(total_fee) as total_fee, COUNT(*) from t_bill_success_flow_%s where order_status = 'SUCCESS' AND "
		" pay_time >= '%s'  AND  pay_time <= '%s' GROUP BY mch_id ",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG("GetPayBillData sql: %s", sql_stmt);

	stGetPayBillData_Resp pGetPay[row_count];

	iRet = spp_bill_GetPayBillData(0,&param,pGetPay,sql_stmt,row_count);

	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetPayBillData Execute Failed.Ret[%d] ",
			iRet);
		return -20;
	}

	for (int i = 0; i < row_count; i++)
	{
		OrderPayBillSumary orderPayBill;
		orderPayBill.Reset();
		orderPayBill.mch_id = pGetPay[i].mch_id;//(row[0]) ? row[0] : "";
		orderPayBill.shop_amount = pGetPay[i].sum_shop_amount;//(row[1]) ? atoll(row[1]) : 0;
		orderPayBill.channel_profit = pGetPay[i].sum_channel_profit;//(row[2]) ? atoll(row[2]) : 0;
		orderPayBill.service_profit = pGetPay[i].sum_service_profit;//(row[3]) ? atoll(row[3]) : 0;
		orderPayBill.payment_profit = pGetPay[i].sum_payment_profit;//(row[4]) ? atoll(row[4]) : 0;
		orderPayBill.bm_profit = pGetPay[i].sum_bm_profit;//(row[5]) ? atoll(row[5]) : 0;
		orderPayBill.total_commission = pGetPay[i].sum_total_commission;//(row[6]) ? atoll(row[6]) : 0;
		orderPayBill.total_fee = pGetPay[i].sum_total_fee;//(row[7]) ? atoll(row[7]) : 0;
		orderPayBill.trade_count = pGetPay[i].count;//(row[8]) ? atoll(row[8]) : 0;

		rtrim(orderPayBill.mch_id);

		orderPayBillSMap.insert(std::make_pair(orderPayBill.mch_id, orderPayBill));

	}

	return RET_HASREC;


}


int CPayTransSybaseDao::GetRefundBillData(CSyBase& sql_instance,
									const std::string& strBmId, const std::string& strBeginTime, const std::string& strEndTime,
									std::map<std::string, OrderRefundBillSumary>& orderRefundBillSMap)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	int iRet = 0;
	int row_count = 0;
	//MYSQL_ROW row;

	Reset();

	char sql_stmt[4096];
	char sql_count[512];

	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	snprintf(sql_count,sizeof(sql_count),
		" SELECT COUNT(DISTINCT(mch_id)) FROM t_bill_success_flow_%s where order_status = 'REFUND' AND "
		" pay_time >= '%s'  AND  pay_time <= '%s' ",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	iRet = spp_bill_getrecord_count(0,&param,sql_count,&row_count);

	CDEBUG_LOG(" row_count = [%d]",row_count);
	if(row_count <= 0)
	{
		return RET_HASNOREC;
	}

	snprintf(sql_stmt, sizeof(sql_stmt),
		" SELECT mch_id, SUM(shop_amount) as shop_amount, SUM(channel_profit) as channel_profit, "
		" SUM(service_profit) as service_profit, SUM(payment_profit) as payment_profit, "
		" SUM(bm_profit) as bm_profit, SUM(total_commission) as total_commission, "
		" SUM(refund_fee) as refund_fee, COUNT(*) from t_bill_success_flow_%s where order_status = 'REFUND' AND "
		" pay_time >= '%s'  AND  pay_time <= '%s' GROUP BY mch_id ",
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG("GetRefundBillData sql: %s", sql_stmt);

	stGetRefundBillData_Resp pGetRefund[row_count];

	iRet = spp_bill_GetRefundBillData(0,&param,pGetRefund,sql_stmt,row_count);

	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetRefundBillData Execute Failed.Ret[%d] ",
			iRet);
		return -20;
	}

	for (int i = 0; i < row_count; i++)
	{
		OrderRefundBillSumary orderRefundBill;
		orderRefundBill.Reset();
		//if ((row = sql_instance.fetch_row()))
		//{
		orderRefundBill.mch_id = pGetRefund[i].mch_id;//(row[0]) ? row[0] : "";
		orderRefundBill.shop_amount = pGetRefund[i].sum_shop_amount;//(row[1]) ? atoll(row[1]) : 0;
		orderRefundBill.channel_profit = pGetRefund[i].sum_channel_profit;//(row[2]) ? atoll(row[2]) : 0;
		orderRefundBill.service_profit = pGetRefund[i].sum_service_profit;//(row[3]) ? atoll(row[3]) : 0;
		orderRefundBill.payment_profit = pGetRefund[i].sum_payment_profit;//(row[4]) ? atoll(row[4]) : 0;
		orderRefundBill.bm_profit = pGetRefund[i].sum_bm_profit;//(row[5]) ? atoll(row[5]) : 0;
		orderRefundBill.total_commission = pGetRefund[i].sum_total_commission;//(row[6]) ? atoll(row[6]) : 0;
		orderRefundBill.refund_fee = pGetRefund[i].sum_refund_fee;//(row[7]) ? atoll(row[7]) : 0;
		orderRefundBill.refund_count = pGetRefund[i].count;//(row[8]) ? atoll(row[8]) : 0;

		rtrim(orderRefundBill.mch_id);

		orderRefundBillSMap.insert(std::make_pair(orderRefundBill.mch_id, orderRefundBill));

	}

	return RET_HASREC;
}


int CPayTransSybaseDao::GetChannelBillData(CSyBase& sql_instance, const std::string& strTableFix, const std::string& strBmId,
			const std::string& strBeginTime, const std::string& strEndTime, const std::string& order_status, std::map<std::string, int>& channelMap)
{
	CDEBUG_LOG("%s:%s",__FILE__,__func__);
	int iRet = 0;
	int row_count = 0;
	//MYSQL_ROW row;
	Reset();
	//t_order_channel_flow
	char sql_stmt[4096];
	char sql_count[512];

	T_STSYBASE_IN_PARAM param;
	sprintf(param.user,"%s",sql_instance.ms_user);
	sprintf(param.pwd,"%s",sql_instance.ms_pass);
	sprintf(param.host,"%s",sql_instance.ms_host);
	sprintf(param.dbname,"%s",sql_instance.ms_db);

	snprintf(sql_count, sizeof(sql_count),
		" SELECT COUNT(DISTINCT(channel_id)) from %s where order_status = '%s' AND "
		" pay_time >= '%s'  AND  pay_time <= '%s' and order_no in (SELECT order_no from t_bill_success_flow_%s where pay_time >= '%s' and pay_time <= '%s' GROUP BY order_no) GROUP BY channel_id ",
		strTableFix.c_str(), order_status.c_str(), strBeginTime.c_str(), strEndTime.c_str(),
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG("count sql = [%s]",sql_count);
	iRet = spp_bill_getrecord_count(0,&param,sql_count,&row_count);

	CDEBUG_LOG(" row_count = [%d]",row_count);
	if(row_count <= 0)
	{
		return RET_HASNOREC;
	}

	snprintf(sql_stmt, sizeof(sql_stmt),
		" SELECT channel_id, SUM(channel_profit) as channel_profit from %s where order_status = '%s' AND "
		" pay_time >= '%s'  AND  pay_time <= '%s' and order_no in (SELECT order_no from t_bill_success_flow_%s where pay_time >= '%s' and pay_time <= '%s' GROUP BY order_no) GROUP BY channel_id ",
		strTableFix.c_str(), order_status.c_str(), strBeginTime.c_str(), strEndTime.c_str(),
		strBmId.c_str(), strBeginTime.c_str(), strEndTime.c_str());

	CDEBUG_LOG("GetChannelBillData sql: %s", sql_stmt);

	stGetChannelBillData_Resp pGetChannel[row_count];


	iRet = spp_bill_GetChannelBillData(0,&param,pGetChannel,sql_stmt,row_count);
	if (iRet < 0)
	{
		snprintf(m_szErrMsg, sizeof(m_szErrMsg),
			"GetChannelBillData Execute Failed.Ret[%d]",
			iRet);
		return -20;
	}

	for (int i = 0; i < row_count; i++)
	{
		//if ((row = sql_instance.fetch_row()))
		//{
			std::string channel_id = pGetChannel[i].channel_id;//(row[0]) ? row[0] : "";
			int channel_profit = pGetChannel[i].sum_channel_profit;//(row[1]) ? atoll(row[1]) : 0;

			rtrim(channel_id);

			channelMap.insert(std::make_pair(channel_id, channel_profit));

	}

	return RET_HASREC;
}
