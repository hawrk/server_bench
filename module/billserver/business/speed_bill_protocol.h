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

//db
#define PAY_ORDER_DB   			"pay_order_db"
#define BILL_DB        			"bill_db"
//trade table
#define ORDER          			"t_order"
#define ORDER_CHANNEL  			"t_order_channel"
//bill table
#define ORDER_WXPAY_FLOW        "t_order_wxpay_flow"
#define ORDER_WX_CHAN_FLOW      "t_order_channel_wxpay_flow"
#define ORDER_ALIPAY_FLOW       "t_order_alipay_flow"
#define ORDER_ALI_CHAN_FLOW     "t_order_channel_alipay_flow"
#define BILL_SUCC_FLOW          "t_bill_success_flow"
#define BILL_WX_OVERFLOW        "t_wx_overflow"
#define BILL_ALI_OVERFLOW       "t_ali_overflow"


#endif /* ORDER_PROTOCOL_H_ */
