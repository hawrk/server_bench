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

//数据库相关宏定义
#define BASE_DB             "route_conf_db"
#define SHOP_DB				"shop_db"

//表名宏定义
#define ADMIN_PERMISSION    "t_admin_permission"
#define ADMIN_ROLE          "t_admin_role"
#define SHOPS_TABLE         "shops"


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

#define SPDB_BM_ID                        "1006" // 浦发银行ID

#define SPDB_SETTLE_TYPE_NORMAL           "1"     //常规结算
#define SPDB_SETTLE_TYPE_ERROR            "2"     //差错结算

string GetMchKey(const string& mch_id) throw(CTrsExp);

string GetMchSign(const string& mch_id,const string& szSrc) throw(CTrsExp);

void CheckMD5Sign(NameValueMap& nvMap) throw(CTrsExp);

void CallIdServer(NameValueMap& reqMap,NameValueMap& resMap)throw(CTrsExp);

void QryTableLoop(CDBPool* pDbPool,CMySQL &sqlHandle,string & szDbName,string &szTable,string &szSqlBuf1,string &szSqlBuf2,SqlResultMapVector &resultMVecter);

void ParseOrderNo(string & szOrderNo,NameValueMap & resMap);

////
#endif

