/*
 * bill_protocol.h
 *
 *  Created on: 2017年7月19日
 *      Author: hawrkchen
 */

#ifndef _BILL_PROTOCOL_H_
#define _BILL_PROTOCOL_H_

using namespace std;

typedef std::map<std::string, std::string> StringMap;

//配置文件字段
typedef struct _STBillSrvMainConf
{
	string m_spdb_base_url;     //深浦发请求地址
	string m_spdb_bill_path;    //深浦发对账文件路径
	string m_filetodb_shell;     //文件导入DB 脚本
	string m_bill_file_prefix;    //对账文件前缀

	string m_swiftpass_url;     //威富通请求URL
	string m_get_speedpos_bill_shell;
	string m_speedpos_sftp_ip;
	string m_speedpos_sftp_port;
	string m_speedpos_sftp_user;
	string m_speedpos_sftp_pwd;
	string m_speedpos_remote_path;

}STBillSrvMainConf;


typedef struct _tagChannelInfo
{
	string channel_id;
	string channel_name;

	void Reset()
	{
		channel_id = "";
		channel_name = "";
	}
}ChannelInfo;

//本地成功交易数据
typedef struct _tagOrderPayBillSummary
{
	string  order_no;
	string  out_order_no;
	string  agent_id;
	string  mch_id;
	string  pay_channel;
	string  transaction_id;
	string  total_amount;
	string  factor_rate_val;
	string  mch_rate_val;
	string  product_rate_val;
	string  serv_rate_val;
	string  order_status;

	void Reset()
	{
		order_no = "";
		out_order_no = "";
		agent_id = "";
		mch_id = "";
		pay_channel = "";
		transaction_id = "";
		total_amount = "";
		factor_rate_val = "";
		mch_rate_val = "";
		product_rate_val = "";
		serv_rate_val = "";
		order_status = "";
	}
}OrderPayBillSummary;

//本地退款交易数据
typedef struct _tagOrderRefundBillSummary
{
	string  order_no;
	string  out_order_no;
	string  agent_id;
	string  mch_id;
	string  pay_channel;
	string  transaction_id;
	string  refund_amount;
	string  order_status;
	void Reset()
	{
		order_no = "";
		out_order_no = "";
		agent_id = "";
		mch_id = "";
		pay_channel = "";
		transaction_id = "";
		refund_amount = "";
		order_status = "";
	}
}OrderRefundBillSummary;

//银行交易数据
typedef struct _tagBankpayBillSummary
{
	string  bill_date;
	string  sub_mch_id;
	string  product_id;
	string  trade_type;
	string  order_no;
	string  pay_time;
	string  pay_amount;
	string  pay_fee;
	string  settle_amount;
	string  ori_order_no;
	string  ori_pay_time;

	void Reset()
	{
		bill_date = "";
		sub_mch_id = "";
		product_id = "";
		trade_type = "";
		order_no = "";
		pay_time = "";
		pay_amount = "";
		pay_fee = "";
		settle_amount = "";
		ori_order_no = "";
		ori_pay_time = "";
	}
}BankpayBillSummary;

//商户对账单数据
typedef struct _tagMchBillSum
{
	string order_no;
	string mch_id;
	string agent_id;
	string channel_id;
	string channel_name;
	int total_count;
	long total_amount;
	int refund_count;
	long refund_amount;
	long total_fee;
	long net_amount;
	long pending_amount;

	void Reset()
	{
		order_no = "";
		mch_id = "";
		agent_id = "";
		channel_id = "";
		channel_name = "";
		total_count = 0;
		total_amount = 0;
		refund_count = 0;
		refund_amount = 0;
		total_fee = 0;
		net_amount = 0;
		pending_amount = 0;
	}
}MchBillSum;


typedef struct _tagOrderLiquidattion
{
	string  mch_id;
	string  mch_name;
	string  agent_id;
	string  pay_channel;
	int     total_count;      //交易总笔数
	long  total_amount;    //交易总金额
	int     refund_count;    //退款总笔数
	long  refund_amount;   //退款总金额
	long  total_fee;       //总手续费
	long  net_amount;     //交易净额    总金额 - 总退款
	long  mch_net_amount;  //商户结算金额    总金额-总手续费
	long  agent_profit;    //渠道分润所得
	long  bm_profit;       //通道成本
	long  serv_profit;     //平台收益

	void Reset()
	{
		mch_id = "";
		mch_name = "";
		agent_id = "";
		pay_channel = "";
		total_count = 0;
		total_amount = 0;
		refund_count = 0;
		refund_amount = 0;
		total_fee = 0;
		net_amount = 0;
		mch_net_amount = 0;
		agent_profit = 0;
		bm_profit = 0;
		serv_profit = 0;
	}
}OrderPayLiquidate;


#endif /* _BILL_PROTOCOL_H_ */
