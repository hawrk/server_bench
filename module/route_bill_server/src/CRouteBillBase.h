#ifndef _AGENT_PAY_BASE_H_
#define _AGENT_PAY_BASE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sstream>
#include "../Base/Comm/comm_protocol.h"
#include "CExp.h"
#include "mysqlapi.h"
#include "CSpeedPosConfig.h"
#include "errorNo.h"
#include "CSocket.h"
#include "DBPool.h"
#include "json_util.h"
#include <math.h>
#include "bill_protocol.h"
#include "tinyxml2.h"

//数据库相关宏定义
#define BASE_DB             "route_conf_db"
#define ROUTE_ORDER_DB      "ROUTE_ORDER_DB"
#define ROUTE_REFUND_DB     "ROUTE_REFUND_DB"
#define ROUTE_BILL_DB       "route_checkbill_db"

//表名宏定义
#define ORDER_TABLE         "t_order"
#define REFUND_TABLE        "t_refund"
#define ORDER_CHANNEL       "t_factor_fee_info"

#define T_ROUTE_BILL        "t_route_check_bill"
#define T_ROUTE_CHANNEL     "t_route_bill_channel"
#define SZSPDB_CHECK_BILL   "t_route_szspdb_bill"
#define SZSPDB_SETTLE       "t_route_szspdb_settle"
#define BILL_SUMMARY        "t_route_bill_summary"
#define BILL_ABNORMAL       "t_route_bill_abnormal"
#define BILL_DISTRIBUTION   "t_route_bill_distribution"
#define MCH_CHECK_BILL      "t_mch_check_bill"
#define SWIFT_CHECK_BILL    "t_route_swiftpass_bill"
#define SPEEDPOS_CHENK_BILL "t_route_speedpos_bill"


#define ORDER_SUCCESS       "SUCCESS"
#define ORDER_REFUND        "REFUND"

#define FUND_TYPE_MCH       "mch"
#define FUND_TYPE_CH        "factor"
#define FUND_TYPE_BM        "bm"
#define FUND_TYPE_SERV      "serv"


#define ABNORMAL_ORDER_OVER_FLOW       1    //本地多
#define ABNORMAL_BANK_OVER_FLOW        2    //银行多
#define ABNORMAL_AMOUNT_NOT_MATCH      3    //金额不符
#define ABNORMAL_STATE_NOT_MATCH       4    //状态不一致


#define CHECK_BILL_SUCC                1    //平账
#define CHECK_BILL_FAIL                2    //不平

#define ACCOUNT_INIT                 "0"    //未记账
#define ACCOUNT_SUCCESS              "1"    //记账成功
#define ACCOUNT_FAIL                 "2"    //记账失败

/*
 *   深圳浦发请求报文一些宏定义
 *   BEGIN:
 */
#define BANK_VERSION        "V1.1"
#define BANK_TRANSID        "21"
#define DOMAIN_URL          "payment-gate-web/gateway/api/backTransReq"

#define BANK_RET_SUCCESS        "0000"
//#define SUB_MCH_ID          "310440300001111"
/*
 * END
 */

/*
 * SWIFTPASS 威富通接口一些宏定义
 * BEGIN:
 */
#define SERVICE_MCH_TYPE        "pay.bill.merchant"
#define SERVICE_BIGMCH_TYPE     "pay.bill.bigMerchant"
#define SERVICE_AGENT_TYPE      "pay.bill.agent"

#define SWIFT_VERSION           "1.0"
#define SWIFT_CHARSET           "UTF-8"
#define SWIFT_BILL_TYPE         "ALL"
#define SWIFT_SIGN_TYPE         "MD5"

/*
 * END
 */




#define MAX_MONEY                 9999999999       //最大金额

/**********************/
/*********分页相关属性*********/
/**********************/
#define LIMIT_MIN                         1       //最小每页显示记录数
#define LIMIT_MAX                         200     //最大每页显示记录数
#define LIMIT_DEFAULT                     10      //默认缺省情况下每页显示记录数
#define PAGE_MIN                          1       //最小查询起始页数
#define PAGE_MAX                          99999999//最大查询起始页数
#define PAGE_DEFAULT                      1       //默认查询起始页

#define VER                               "1.0"   //版本号

#define APAY_MAX_RESP_LEN                  102400
#define APAY_SPDB_MAX_BUFFER_LEN           102400


//函数 的一些宏定义
#define AddJsonMap(MAP,KEY,VALUE)  \
	do \
	{ \
		MAP.insert(JsonMap::value_type(JsonType(KEY), JsonType(VALUE))); \
	}while(0)


#define GETJSONVALUE(key, ctJsonMap)			GetJsonValue(key, ctJsonMap)

//end

string GetMchKey(const string& mch_id) throw(CTrsExp);

string GetMchSign(const string& mch_id,const string& szSrc) throw(CTrsExp);


void CallIdServer(NameValueMap& reqMap,NameValueMap& resMap)throw(CTrsExp);

void QryTableLoop(CDBPool* pDbPool,CMySQL &sqlHandle,string & szDbName,string &szTable,string &szSqlBuf1,string &szSqlBuf2,SqlResultMapVector &resultMVecter);

void ParseOrderNo(string & szOrderNo,NameValueMap & resMap);

void ParseJson2Map(const string& jsonStr, NameValueMap& strMap);

string GetJsonValue(const string& key, const JsonMap& ctJsonMap);


int  SetOneFieldToXml(tinyxml2::XMLDocument * pDoc, tinyxml2::XMLNode* pXmlNode, const char * pcFieldName,
    const char* pszValue, bool bIsCdata);

////
#endif

