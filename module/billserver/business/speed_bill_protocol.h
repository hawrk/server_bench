/*
 * order_protocol.h
 *
 *  Created on: 2010-5-24
 *      Author: rogeryang
 */

#ifndef SPEED_ORDER_PROTOCOL_H_
#define SPEED_ORDER_PROTOCOL_H_

#include <string>
using namespace std;
#include "comm_protocol.h"
#include "../../Base/Comm/groupon_common.h"

#include "../../Base/fw/struct.h"

const INT32 CMD_SPEEDPOS_PSUH_SYS_BILL = 5010;   //对账单生成和下载

const INT32 CMD_SPEEDPOS_PSUH_WX_API_BILL = 5020;

const INT32 CMD_SPEEDPOS_PSUH_ALI_API_BILL = 5030;

const INT32 CMD_SPEEDPOS_CONTRAST_BILL = 5040;  //对账操作

const INT32 CMD_SPEEDPOS_SETTLE_CALLBACK = 5050;   //清分结果回调

const INT32 CMD_SPEEDPOS_BILL_DOWNLOAD = 5060;   //对账单查询

const INT32 CMD_SPEEDPOS_BILL_BATCH   = 5070;    //对账单批次处理

const INT32 CMD_SPEEDPOS_BILL_SUMMARY  = 5080;   //对账汇总查询

const INT32 CMD_SPEEDPOS_BILL_ABNORMAL  = 5090;   //对账差错查询

const INT32 CMD_SPEEDPOS_BILL_SETTLE_QRY  = 5100;   //结算查询

const INT32 CMD_SPEEDPOS_APAY_BILL_DEAL  = 5110;   //代付对账处理

const INT32 CMD_SPEEDPOS_APAY_ABNOR_BILL_QRY  = 5120;   //代付对账异常订单查询

const INT32 CMD_SPEEDPOS_SETTLE_ACCTINFO_MODIFY  = 5130;   //结算账户信息编辑

const INT32 CMD_SPEEDPOS_APAY_BILL_LOG_QRY  = 5140;   //代付对账记录查询

#define MAX_MONEY                 		9999999999       //最大金额
#define LIMIT_MIN                         1       //最小每页显示记录数
#define LIMIT_MAX                         200     //最大每页显示记录数
#define LIMIT_DEFAULT                     10      //默认缺省情况下每页显示记录数
#define PAGE_MIN                          1       //最小查询起始页数
#define PAGE_MAX                          99999999//最大查询起始页数
#define PAGE_DEFAULT                      1       //默认查询起始页

#define VERSION                 "1.0"


//db
#define PAY_ORDER_DB   			"pay_order_db"
#define BILL_DB        			"bill_db"
//trade table
#define ORDER          			"t_order"
#define ORDER_CHANNEL  			"t_order_channel"
//bill table
#define ORDER_ALL_FLOW          "t_order_all_flow"
#define ORDER_CHANNEL_FLOW      "t_order_channel_all_flow"
#define ORDER_WXPAY_FLOW        "t_order_wxpay_flow"
#define ORDER_WX_CHAN_FLOW      "t_order_channel_wxpay_flow"
#define ORDER_ALIPAY_FLOW       "t_order_alipay_flow"
#define ORDER_ALI_CHAN_FLOW     "t_order_channel_alipay_flow"
#define BILL_SUCC_FLOW          "t_bill_success_flow"
#define BILL_WX_OVERFLOW        "t_wx_overflow"
#define BILL_ALI_OVERFLOW       "t_ali_overflow"
#define BILL_WXPAY_FLOW         "t_wxpay_flow"
#define BILL_ALIPAY_FLOW        "t_alipay_flow"

#define BILL_DOWNLOAD           "t_bill_download"
#define BILL_BATCH              "t_bill_batch_manage"
#define BILL_SUMMARY            "t_bill_summary"
#define BILL_ABNORMAL           "t_bill_abnormal"
#define BILL_DISTRIBUTION       "t_bill_distribution"
#define BILL_SETTLE             "t_bill_settle"


#endif /* ORDER_PROTOCOL_H_ */
