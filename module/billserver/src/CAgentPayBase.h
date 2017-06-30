#ifndef _AGENT_PAY_BASE_H_
#define _AGENT_PAY_BASE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sstream>
#include "../Base/Comm/comm_protocol.h"
#include "CExp.h"
#include "../common/mysqlapi.h"
#include "CSpeedPosConfig.h"
#include "../business/apayErrorNo.h"
#include "CSocket.h"
#include "DBPool.h"
#include "json_util.h"
#include "../common/CCommFunc.h"

//数据库相关宏定义
#define SHOP_DB             "shop_db"
#define AGENT_PAY_DB        "agentpay_conf_db"
#define AGENT_PAY_ORDER_DB  "agentpay_order_db"
#define AGENT_PAY_BILL_DB  "agentpay_bill_db"

#define SHOPS_TABLE            "shops"
#define SUBUSER_TABLE          "subusers"
#define APAY_MCH_CONF_TABLE    "t_agentpay_mch_info"
#define APAY_CHL_CONF_TABLE    "t_agentpay_channel_info"
#define APAY_ORDER_TABLE       "t_agentpay_order"
#define APAY_WEBANK_BILL_TABLE  "t_webank_bill_temp"
#define APAY_WEBANK_REFUND_BILL_TABLE  "t_webank_refund_bill_temp"
#define APAY_WEBANK_BILL_ABNORMAL_TABLE "t_webank_bill_abnormal"
#define APAY_BILL_LOG_TABLE      "t_webank_bill_log"


//subuser状态相关定义
#define SUBUSER_STATUS_NORMAL       "1"      //禁用
#define SUBUSER_STATUS_FORBIDDEN    "-1"     //正常


//商户状态相关定义
#define SHOP_STATUS_FORBIDDEN       "-1"       //禁用
#define SHOP_STATUS_NORMAL          "1"        //正常

//代付商户状态
#define APAY_MCH_STATUS_NORMAL      "2"     //启用
#define APAY_MCH_STATUS_STOPED      "1"     //未启用


//代付渠道状态定义
#define APAY_CHL_STATUS_STOPED      "1"     //未启用
#define APAY_CHL_STATUS_NORMAL      "2"     //启用


//代付单状态宏定义
#define APAY_ORDER_STATUS_DEALING    "1"     //处理中
#define APAY_ORDER_STATUS_SUCCESS    "2"     //成功
#define APAY_ORDER_STATUS_FAILED     "3"     //失败
#define APAY_ORDER_STATUS_REFUND     "4"     //退票

//代付单物理状态宏定义
#define APAY_ORDER_PYH_STATUS_VALID       "1"     //有效
#define APAY_ORDER_PYH_STATUS_INVALID     "2"     //无效
#define APAY_ORDER_PYH_STATUS_DELETED     "3"     //删除

//订单同步状态宏定义
#define APAY_ORDER_SYNC_DONE     "1"     //已同步
#define APAY_ORDER_SYNC_WAIT     "2"     //等待同步

//订单异步通知标识宏定义
#define APAY_NOTIFY_FLAG_SUCC   "1"      //成功
#define APAY_NOTIFY_FLAG_FAIL   "2"      //失败


//对账状态宏定义
#define APAY_BILL_STATUS_DEALING    "1"   //对账中
#define APAY_BILL_STATUS_SUCCESS    "2"   //对账成功
#define APAY_BILL_STATUS_FAILED     "3"   //对账失败


//微众订单状态宏定义
#define WEBANK_ORDER_STATUS_DEALING       "PR00"
#define WEBANK_ORDER_STATUS_SUCCESS       "PR03"
#define WEBANK_ORDER_STATUS_FAILED        "PR02"


 

//代付通道编码
#define APAY_CHL_SPDB               "1"    //上海浦发
#define APAY_CHL_WEBANK             "1001" //微众银行

#define WEBANK_VERSION             "2.0.0"


//订单类型
#define ORDER_TYPE_CHARGE           "1"   //充值
#define ORDER_TYPE_DRAW             "2"   //提现
#define ORDER_TYPE_APAY             "3"   //代付


//业务模式
#define BIZ_TYPE_NORMAL             "1"    //常规模式即代理模式
#define BIZ_TYPE_FUNDS              "2"    //资金次模式即大商户模式

//代付收款账户类型
#define APAY_ACCT_TYPE_PRI          "1"    //对私
#define APAY_ACCT_TYPE_PUB          "2"    //对公



#define  APAY_SERVER_ORDER_MODIFY       "6017"       //代付服务-订单更新

#define MAX_MONEY                 9999999999       //最大金额

/**********************/
/*********分页相关属性*********/
/**********************/
#define LIMIT_MIN                         1       //最小每页显示记录数
#define LIMIT_MAX                         200     //最大每页显示记录数
//#define LIMIT_DEFAULT                     10      //默认缺省情况下每页显示记录数
#define PAGE_MIN                          1       //最小查询起始页数
#define PAGE_MAX                          99999999//最大查询起始页数
//#define PAGE_DEFAULT                      1       //默认查询起始页

#define VER                               "1.0"   //版本号



string GetMchKey(const string& mch_id) throw(CTrsExp);
string GetMchSign(const string& mch_id,const string& szSrc) throw(CTrsExp);

void CheckMD5Sign(NameValueMap& nvMap) throw(CTrsExp);

void CallAgentPayServer(NameValueMap& reqMap,NameValueMap& resMap)throw(CTrsExp);

void QryTableLoop(CDBPool* pDbPool,CMySQL &sqlHandle,string & szDbName,string &szTable,string &szSqlBuf1,string &szSqlBuf2,SqlResultMapVector &resultMVecter);

INT32 CallRabbitMQNew(NameValueMap & reqMap);

INT32 CallRabbitMQ(JsonMap& reqMap);

void ParseOrderNo(string & szOrderNo,NameValueMap & resMap);

////
#endif

