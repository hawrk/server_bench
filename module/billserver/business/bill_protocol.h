/**
 * @file: pay_order.h
 * @brief: 定义支付系统订单所需要结构体
 * @Author: lining
 * @Created on: 星期一,  22 8月 2016.
 */
 
#ifndef BILL_PROTOCOL_H_
#define BILL_PROTOCOL_H_

#include <string.h>
#include <map>

#include "../typedef.h"
#include "../types.h"
#include "../../Base/fw/struct.h"
#include "../../Base/Comm/comm_protocol.h"
using namespace std;

#ifdef _KOREA_VER

#define  BILL_FILE_TABLE_HEAD                           "은행 번호,거래시간,플랫폼 주문 번호,제3자 주문 번호,지급 시스템 거래 번호,프랫폼 가맹점 번호,채널ID,지급 채널,지급 유형,거래 상황,거래 금액,총 수수료,가맹점 소득,환불 금액,플랫폼 환불 홀수 번호,제3자 환불 홀수 번호,지급 시스템 환불 홀수 번호,지급 수수료,채널 총 중개 수수료,데이터 정리 기관 중개 수수료,기술 봉사료"
#define  CHANNEL_BILL_FILE_TABLE_HEAD                   "은행 번호,거래시간,플랫폼 주문 번호,프랫폼 가맹점 번호,채널ID,지급 채널,지급 유형,거래 상황,거래 금액,환불 금액,플랫폼 환불 홀수 번호,채널 결산 비용률,채널 중개 수수료"
#define SHOP_REMARK_NAME		"가맹점 거래당 금액"
#define CHANNEL_REMARK_NAME		"채널 수수료"
#define SERVICE_REMARK_NAME		"기술 수수료"
#define BM_REMARK_NAME		    "기관 수수료"
#else
#define  BILL_FILE_TABLE_HEAD                           "银行编号,交易时间,平台订单号,第三方订单号,支付渠道交易号,平台商户号,渠道ID,支付渠道,支付类型,交易状态,交易金额,总手续费,商户结算金额,退款金额,平台退款单号,第三方退款单号,支付渠道退款单号,支付手续费,渠道总佣金,清分机构佣金,技术服务费,\
账单状态,货币类型,附言,结算费率"
#define  CHECKED_BILL_FILE_HEAD                           "银行编号,交易时间,平台订单号,第三方订单号,支付渠道交易号,平台商户号,渠道ID,支付渠道,支付类型,交易状态,交易金额,总手续费,商户结算金额,退款金额,平台退款单号,第三方退款单号,支付渠道退款单号,支付手续费,渠道总佣金,清分机构佣金,技术服务费,附言,"

#define  CHANNEL_BILL_FILE_TABLE_HEAD                   "银行编号,交易时间,平台订单号,平台商户号,渠道ID,支付渠道,支付类型,交易状态,交易金额,退款金额,平台退款单号,渠道结算费率,渠道佣金"
#define SHOP_REMARK_NAME		"商户收单"
#define CHANNEL_REMARK_NAME		"渠道手续费"
#define SERVICE_REMARK_NAME		"技术手续费"
#define BM_REMARK_NAME		    "机构手续费"
#endif

#define RESP_SUCCUSS_MSG  "SUCCESS"

#define SHOP_TYPE_NAME			"mch"
#define CHANNEL_TYPE_NAME		"ch"
#define SERVICE_TYPE_NAME		"serv"
#define BM_TYPE_NAME			"bm"

#define  WX_API_PAY_CHANNEL      "WXPAY"
#define  ALI_API_PAY_CHANNEL     "ALIPAY"
#define  SYS_API_PAY_CHANNEL     "SYSTEM"

//对账和结算单文件存放目录
#define SRC_PATH         "src/"
#define ENCRYPT_PATH     "encrypt/"


//wxpay && alipay url
#define WX_DOWNLOAD_URL       "https://api.mch.weixin.qq.com/pay/downloadbill"
#define ALI_DOWNLOAD_METHOD   "alipay.data.dataservice.bill.downloadurl.query"

#define SQL_TRY_BEG() try {
#define SQL_TRY_END() }catch(CTrsExp& e){\
    m_jsonRsp[JsonType("error")] = JsonType("1");\
    m_jsonRsp[JsonType("msg")] = JsonType(e.retmsg);\
    CERROR_LOG("mysql Execute occurs CTrsExp exception, err_code:%s,err_msg:%s", e.retcode.c_str(), e.retmsg.c_str());\
    return;\
}catch(...){\
    m_jsonRsp[JsonType("error")] = JsonType("2");\
    m_jsonRsp[JsonType("msg")] = JsonType("bill db ... error");\
    CERROR_LOG("mysql Execute occurs ...exception");\
    return;\
}\

/** STL map default sort order by key **/
typedef std::map<std::string, std::string> StringMap;

enum ENUM_REFUND_ORDER_STATUS {
	REFUND_ORDER_STATUS_FAIL = -3,     // 退款失败
	REFUND_ORDER_STATUS_CREATE = 1,     // spp申请退款
	REFUND_ORDER_STATUS_APPLY_SUCESS = 2,     // wx退款申请成功
	REFUND_ORDER_STATUS_SUCC = 3,     // 退款成功
	REFUND_ORDER_STATUS_PROCESSING = 4, //退款处理中
	REFUND_ORDER_STATUS_CHANGEG = 5, //转入代发 退款到银行发现用户的卡作废或者冻结了，导致原路退款银行卡失败，资金回流到商户的现金帐号，需要商户人工干预，通过线下或者财付通转账的方式进行退款。
};

enum ENUM_BILL_TYPE{
	BILL_TYPE_WX = 1,     // 
	BILL_TYPE_SPEEDPOS = 2,     // 
	BILL_TYPE_CHANNEL = 3,     // 
};


typedef struct _STBillSrvMainConf
{
	int 		nCheckWay;				//对账方式: 0 行内, 1 行外

	// 远程FTP 信息
	string 		sFtpIp;
	string 		sFtpUser;
	string 		sFtpPass;
	int         iSftpPort;

	//阿里对账单下载URL地址
	string      sAliGateWayUrl;
	string      sAliDetailSuffix;

	//微信账单方面的配置信息
	//string		sWXBillSrcPath; 		//微信的原始账单目录  :/XXX/
	//string		sWXBillEncPath; 		//微信加密账单目录	  :/YYY/
	string      sWXBillPath;             //微信账单目录
	string		sWXBillSrcPrefix;		//微信未加密账单的名字
	string		sWXBillEncPrefix;		//微信加密账单的名字

	//支付宝账单方面的配置信息
	//string		sAliBillSrcPath; 		//微信的原始账单目录  :/XXX/
	//string		sAliBillEncPath; 		//微信加密账单目录	  :/YYY/
	string 	    sAliBillPath;            //支付宝账单目录
	string		sAliBillSrcPrefix;		//微信未加密账单的名字
	string		sAliBillEncPrefix;		//微信加密账单的名字

	//商户原始账单文件
	string 		sMchBillSrcFilePrefix;
	//string 		sMchBillSrcPath;
	string 		sMchBillEncFilePrefix;
	//string 		sMchBillEncPath;
	string      sMchBillPath;



	// 渠道原始账单文件
	string		sChannelBillSrcFilePrefix;
	//string		sChannelBillSrcPath;
	string		sChannelBillEncFilePrefix;
	//string		sChannelBillEncPath;
	string      sChannelBillPath;

	//
	std::string sSftpLongRangPath;
	//std::string sSftpBillSrcPath;
	//std::string sSftpBillDecPath;
	string      sSftpBillPath;
	std::string sSftMchBillFilePrefix;
	std::string sSftChannelBillFilePrefix;
	std::string sSftWXBillFilePrefix;
	std::string sSftAliBillFilePrefix;

	std::string sSftpShellPath;
	std::string sBillFileToDBShellPath;
	std::string sBillTruncateDBShellPath;

	//打款单信息
	string 		sRemittancePath;
	//string		sRemittanceSrcPath;
	//string      sRemittanceEncryptPath;
	string		sRemittanceSrcFilePrefix;
	//string		sRemittanceResultSrcPath;
	string		sRemittanceResultSrcPrefix;
	//string 		sRemittanceBankResultPath;


	std::string   sGetBankNoApiUrl;
	std::string   sAddSettleLogUrl;
	std::string   sUpdateSettleLogUrl;
	std::string   sGetPayFailUrl;
	std::string   sAddBillContrastUrl;
	std::string   sUpdateBillContrastUrl;
	std::string   sAddExceptionOrderUrl;
	std::string   sTradeServQryOrderUrl;
	std::string   sApiKey;
	std::string   sSignKey;


	std::string   sCallNotifyIp;
	int    iCallNotifyPort;
	//add hawrk
	std::string sCallSettleSerIp;
	int    iCallSettleSerPort;

	// db info
	string 		sOrderDBHost;
	int			nOrderDBPort;
	string		sOrderDBUser;
	string		sOrderDBPass;
	string		sOrderDBNamePrifix;


	// 技术服务商信息
	string 		service_name_no;
	string		service_name;
	string 		service_name_tag;
	string		service_bank;
	string		service_bank_account;

	// DES 加解密密码
	string 		des_key;


} STBillSrvMainConf;


typedef struct _tagProPullBillReq
{
	CommHead        stHead;
	CHAR            szVersion[32];//版本号
	std::string     sBmId;
	std::string     sPayChannel;
	std::string     sInputTime;
	std::string     sEndTime;
	std::string     sFileName;
	std::string     sOrderType;
	std::string     sStep;
	std::string     sBankInscode;
	std::string     sBillStatus;
	std::string     sOperator;
	std::string     sBatchNo;
	int             page;
	int             limit;
	void Reset()
	{
		memset(&stHead, '\0', sizeof(CommHead));
		memset(&szVersion, '\0', sizeof(szVersion));
		sBmId = "";
		sPayChannel = "";
		sInputTime = "";
		sEndTime = "";
		sFileName = "";
		sOrderType = "";
		sStep = "";
		sBankInscode = "";
		sBillStatus = "";
		sOperator = "";
		sBatchNo = "";
		page = 0;
		limit = 0;
	}
}ProPullBillReq;


typedef struct _tagBillServerResponse
{
	int  err_code;
	std::string err_msg;
	std::string     sReturnContent;

	void Reset()
	{
		err_code = 0;
		err_msg  = "";
		sReturnContent = "";
	}
}BillServerResponse;


/**
* 商户订单交易信息
*/
typedef struct _tagOrderFlowData
{
	std::string     mch_id;         // 商户id
	std::string     channel_id;
	int     order_status;
	int     total_fee;
	int     shop_amount;
	int     total_commission;
	int     refund_fee;
	int     payment_profit;
	int     channel_profit;
	int     service_profit;
	int     bm_profit;
	int     pay_time;
	int     shop_calc_rate;

	std::string   order_no;
	std::string   out_trade_no;
	std::string   trade_type;
	std::string   pay_channel;
	std::string   transaction_id;
	std::string   fee_type;
	std::string   sub_body;

	void Reset()
	{
		mch_id = "";
		channel_id = "";
		order_status = 0;
		total_fee = 0;
		shop_amount = 0;
		total_commission = 0;
		refund_fee = 0;
		payment_profit = 0;
		channel_profit = 0;
		service_profit = 0;
		bm_profit = 0;
		pay_time = 0;
		shop_calc_rate = 0;

		order_no = "";
		out_trade_no = "";
		trade_type = "";
		pay_channel = "";
		transaction_id = "";
		fee_type = "";
		sub_body = "";
	}
}OrderFlowData;

typedef struct _tagOrderChannelFlowData
{
	std::string mch_id;
	std::string channel_id;
	int total_fee;
	int channel_profit_rate;
	int channel_profit;
	int pay_time;
	int total_count;
	int refund_fee;

	std::string order_no;

	void Reset()
	{
		mch_id = "";
		channel_id = "";
		total_fee = 0;
		channel_profit_rate = 0;
		channel_profit = 0;
		pay_time = 0;
		total_count = 0;
		order_no = "";
		refund_fee = 0;
	}
}OrderChannelFlowData;

typedef struct _tagOrderRefundFlowData
{
	std::string     mch_id;         // 商户id
	std::string     channel_id;
	int     status;
	int     total_fee;
	int     refund_fee;
	int     refund_shop_amount;
	int     refund_total_commission;
	int     refund_payment_profit;
	int     refund_channel_profit;
	int     refund_service_profit;
	int     refund_bm_profit;
	int     refund_time;
	int     shop_calc_rate;

	std::string    order_no;
	std::string    refund_no;
	std::string    out_refund_no;
	std::string    refund_id;
	std::string    trade_type;
	std::string    pay_channel;
	std::string   fee_type;
	std::string   sub_body;

	void Reset()
	{
		mch_id = "";
		channel_id = "";
		status = 0;
		total_fee = 0;
		refund_fee = 0;
		refund_shop_amount = 0;
		refund_total_commission = 0;
		refund_payment_profit = 0;
		refund_channel_profit = 0;
		refund_service_profit = 0;
		refund_bm_profit = 0;
		refund_time = 0;
		shop_calc_rate = 0;

		order_no = "";
		refund_no = "";
		out_refund_no = "";
		refund_id = "";
		trade_type = "";
		pay_channel = "";
		fee_type = "";
		sub_body = "";

	}

}OrderRefundFlowData;

typedef struct _tagOrderRefundChannelFlowData
{
	std::string     mch_id;         // 商户id
	std::string     channel_id;
	int     refund_time;
	int     refund_fee;
	int     refund_channel_profit;

	std::string   refund_no;

	void Reset()
	{
		mch_id = "";
		channel_id = "";
		refund_time = 0;
		refund_fee = 0;
		refund_channel_profit = 0;
	}

}OrderRefundChannelFlowData;


typedef struct _tagOrderPayBillSumary
{
	std::string		mch_id;
	int		shop_amount;
	int     total_fee;
	int     payment_profit;
	int     channel_profit;
	int     service_profit;
	int     bm_profit;
	int     total_commission;
	int     trade_count;

	void Reset()
	{
		mch_id = "";
		shop_amount = 0;
		total_fee = 0;	
		payment_profit = 0;
		channel_profit = 0;
		service_profit = 0;
		bm_profit = 0;
		total_commission = 0;
		trade_count = 0;
	}
}OrderPayBillSumary;


typedef struct _tagOrderRefundBillSumary
{
	std::string		mch_id;
	int		shop_amount;
	int     refund_fee;
	int     payment_profit;
	int     channel_profit;
	int     service_profit;
	int     bm_profit;
	int     refund_count;
	int     total_commission;
	void Reset()
	{
			mch_id = "";
			shop_amount = 0;
			refund_fee = 0;			
			payment_profit = 0;
			channel_profit = 0;
			service_profit = 0;
			bm_profit = 0;
			refund_count = 0;
			total_commission = 0;
	}
}OrderRefundBillSumary;

typedef struct _tagOrderStat{

	std::string     mch_id;         // 商户id
	std::string     channel_id;
	int     trade_count;    // 交易总笔数
	int     trade_amount;  //交易总金额(分)
	int     refund_count;   //退款总笔数
	int     refund_amount;   // 退款总额(分)
	int     trade_net_amount; // 交易总净额
	int     total_net_commission; // 总手续费
	int     needpay_amount;  //待结算金额
	int     shop_net_amount;
	int     payment_net_profit;
	int     channel_net_profit;
	int     service_net_profit;
	int     bm_net_profit;
	int     shared_profit;  //分润金额

	void  Reset()
	{
		mch_id = "";
		channel_id = "";
		trade_count = 0;
		trade_amount = 0;
		refund_count = 0;
		refund_amount = 0;
		trade_net_amount = 0;
		total_net_commission = 0;
		needpay_amount = 0;
		shop_net_amount = 0;
		payment_net_profit = 0;
		channel_net_profit = 0;
		service_net_profit = 0;
		bm_net_profit = 0;
		shared_profit = 0;
	}
}OrderStat;



//t+1 汇款单
typedef struct _tagTRemitBill
{
	int             iIndex;
	std::string		account_id;
	int			    remit_fee;
	std::string     sRemitfee;
	std::string		sType;
	std::string		sBankCardNo;
	std::string		sBankOwner;
	std::string     sBankCardType;
	std::string     sBankType;
	std::string     sBranchNo;
	std::string		sName;
	std::string		sPayTime;
	std::string		sRemitTime;
	std::string     sRetCode;
	std::string     sRemark;
	std::string     sPayRemark;
	std::string     sShopName;  //商户名称
	std::string     sCycle;  //结算周期
	std::string     sBankFlag;   //本行跨行标识

	void Reset()
	{
		iIndex = 0;
		account_id = "";
		remit_fee = 0;
		sRemitfee = "";
		sType = "";
		sBankCardNo = "";
		sBankOwner = "";
		sBankCardType = "";
		sBankType = "";
		sName = "";
		sPayTime = "";
		sRemitTime = "";
		sRetCode = "";
		sRemark = "";
		sPayRemark = "";
		sShopName = "";
		sCycle = "";
		sBankFlag = "";
	}
}TRemitBill;


typedef struct _tagWxFlowSummary
{
	string     bm_id;
	string     app_id;         // 商户id
	std::string     mch_id;
	std::string     sub_mch_id;
	string     order_no;
	string     transaction_id;
	string     openid;
	string     order_status;
	string     bank_type;
	string     fee_type;
	long     total_fee;
	long     refund_fee;
	string     refund_id;
	string     refund_no;
	string     pay_time;//SUCCESS 就是支付时间  REFUND 就是退款时间
	string     trade_type;
	string     refund_status;
	string     overflow_type;


//	void Reset()
//	{
//		bm_id = "";
//		pay_time = "";
//		app_id = "";
//		mch_id = "";
//		sub_mch_id = "";
//		order_no = "";
//		transaction_id = "";
//		openid = "";
//		order_status = "";
//		total_fee = 0.00;
//		refund_fee = 0.00;
//		refund_id = "";
//		refund_no = "";
//		trade_type = "";
//		refund_status = "";
//		bank_type = "";
//		fee_type = "";
//	}
}WxFlowSummary;

typedef struct _tagCommonResult
{
	int ret_code;
	std::string ret_msg;

	void Reset()
	{
		ret_code = 0;
		ret_msg = "";
	}
}CommonResult;



typedef struct _tagSOrderInfoRsp
{
	CommonResult result;
	std::string mch_id;
	int total_fee;
	int pay_time;
	int order_status;
	std::string order_no;
	std::string out_trade_no;
	std::string spbill_create_ip;
	std::string sub_openid;
	std::string sub_is_subscribe;
	std::string trade_type;
	std::string bank_type;
	std::string fee_type;
	std::string attach;
	std::string device_info;
	void Rest()
	{
		mch_id = "";
		total_fee = 0;
		pay_time = 0;
		order_status = 0;
		order_no = "";
		out_trade_no = "";
		spbill_create_ip = "";
		sub_openid = "";
		sub_is_subscribe = "";
		trade_type = "";
		bank_type = "";
		fee_type = "";
		attach = "";
		device_info = "";
	}
}SOrderInfoRsp;

typedef struct _tagAliFlowSummary
{
	std::string     mch_id;
	std::string     transaction_id;
	std::string     order_no;
	std::string     order_status;
	std::string     refund_no;
	std::string     pay_time;//SUCCESS 就是支付时间  REFUND 就是退款时间
	std::string     overflow_type;
	long            total_fee;

	void Reset()
	{
		mch_id = "";
		transaction_id = "";
		order_no = "";
		order_status = "";
		refund_no = "";
		pay_time = "";
		overflow_type = "";
		total_fee = 0;
	}
}AliFlowSummary;

typedef struct _ApayBillSrvMainConf
{
	string 		sFtpIp;
	string 		sFtpUser;
	string 		sFtpPass;
	int         iSftpPort;

	string      loadBillFileShPath;
	string      loadBillFileShName;

	string      MchBillFilePath;

}ApayBillSrvMainConf;

//hawrk 当日成功的对账单字段
typedef struct _CheckedBillData
{
	string bm_id;
	string pay_time;
	string order_no;
	string out_order_no;
	string transaction_id;
	string mch_id;
	string channel_id;
	string pay_channel;
	string trade_type;
	string order_status;
	string total_fee;
	string total_commission;
	string shop_amount;
	string refund_fee;
	string refund_no;
	string out_refund_no;
	string refund_id;
	string payment_profit;
	string channel_profit;
	string bm_profit;
	string service_profit;
	string sub_body;

	void Reset()
	{
		bm_id = "";
		pay_time = "";
		order_no = "";
		out_order_no = "";
		transaction_id = "";
		mch_id = "";
		channel_id = "";
		pay_channel = "";
		trade_type = "";
		order_status = "";
		total_fee = "";
		total_commission = "";
		shop_amount = "";
		refund_fee = "";
		refund_no = "";
		out_refund_no = "";
		refund_id = "";
		payment_profit = "";
		channel_profit = "";
		bm_profit = "";
		service_profit = "";
		sub_body = "";
	}
}CheckedBillData;


struct DistriBillOverview
{
	char beg_date[16];
	char end_date[16];
	char bm_id[64];
	void Reset()
	{
		memset(this, 0, sizeof(*this));
	}
};

struct stDistriBillP
{
	string beg_date;
	string end_date;
	string bm_id;
	string pay_channel;
	string mch_id;
	string mch_name;
	int type;
	string shared_type;

	int page;
	int page_count;
	void Reset()
	{
		beg_date = "";
		end_date = "";
		bm_id = "";
		pay_channel = "";
		mch_id = "";
		mch_name = "";
		type = 0;
		page = 0;
		page_count = 0;
	}
};

struct DistriBillDetail
{
	string beg_date;
	string end_date;
	string bm_id;
	string pay_channel;
	string bank_acount;
	string net_no;
	string mch_id;
	string mch_name;
	int type;
	int page;
	int page_count;
	void Reset()
	{
		beg_date = "";
		end_date = "";
		bm_id = "";
		pay_channel = "";
		bank_acount = "";
		net_no = "";
		mch_id = "";
		mch_name = "";
		type = 0;
		page = 0;
		page_count = 0;
	}
};

#endif

