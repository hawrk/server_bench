#ifndef _ERROR_NO_H_
#define _ERROR_NO_H_

#define   SYSTEM_ERR                200001             //系统错误
#define   QRY_DB_ERR               200002             //数据库查询错误
#define   INSERT_DB_ERR             200003             //数据库插入错误
#define   UPDATE_DB_ERR             200004             //数据库更新错误
#define   ERR_RECORD_EXIST            200005             //数据库记录已存在
#define   ERR_PARSE_XML_FAIL          200006             //解析xml失败
#define   ERR_PARSE_JSON_FAIL         200007             //解析Json失败


#define   ERR_INVALID_PARAMS          200201             //关键入错错误
#define   ERR_MCH_STATUS              200202             //商户状态有误

#define   ERR_PAY_CHL_EXISTS          200203             //代付机构已存在
#define   ERR_PAY_CHL_NOT_EXISTS      200204             //代付机构不存在
#define   ERR_APAY_MCH_EXIST          200205             //代付商户已存在无需新增
#define   ERR_MCH_NOT_EXISTS          200206             //商户不存在
#define   ERR_CHECK_MCH_SIGN_FAIL     200207             //校验商户签名失败
#define   ERR_CHL_STATUS              200208             //代付渠道状态有误
#define   ERR_MCH_ORDER_EXIST         200209             //商户单号已存在
#define   ERR_APAY_ORDER_NOT_EXISTS   200210             //代付订单不存在
#define   ERR_APAY_ORDER_STATUS       200211             //代付订单状态有误
#define   ERR_SUBUSER_NOT_EXISTS      200212             //操作员不存在
#define   ERR_SUBUSER_STATUS          200213             //操作员状态有误
#define   ERR_APAY_ORDER_NOT_ALLOW_SYNC   200214         //当前状态不允许同步
#define   ERR_REQ_CHANNEL_FAIL        200215             //请求渠道失败
#define   ERR_PARSE_MSG_FAIL          200216             //解析报文失败
#define   ERR_CHECK_CHL_SIGN_FAIL     200217             //校验渠道签名失败
#define   ERR_ORDER_AMOUNT_DIFF       200218             //订单金额不一致
#define   ERR_INVALID_NOTIFY_DATA     200219             //非法的异步通知数据
#define   ERR_NOTIFY_MCH_FAILED       200220             //通知商户失败
#define   ERR_CHL_PRI_KEY_NOT_EXIST   200221             //渠道私钥不存在
#define   ERR_CHL_PUB_KEY_NOT_EXIST   200222             //渠道公钥不存在
#define   ERR_MCH_FEE_NEED_HIGHER     200223             //商户手续费不能低于渠道手续费
#define   ERR_APAY_TRADE_FAILED       200224             //代付交易失败
#define   ERR_PAY_ACC_ID_NOT_EXIST    200225             //商户代付号有误
#define   ERR_PAY_ACC_NO_NOT_EXIST    200226             //商户代付账号有误
#define   ERR_CALL_ID_SERVER_FAIL     200227             //调用ID server失败


/*浦发清结算*/
#define   ERR_SPDB_ACCT_NO_EXIST                 200301             //资金账户已存在
#define   ERR_SPDB_ACCT_NO_NOT_EXIST             200302             //资金账户不存在
#define   ERR_SPDB_PAY_TNL_EXIST                 200303             //代付通道已存在
#define   ERR_SPDB_PAY_TNL_NOT_EXIST             200304             //代付通道不存在
#define   ERR_DETAIL_NUM_NOT_MATCH               200305             //结算记录数与实际不符
#define   ERR_BILL_STLE_STATUS_ERROR             200306             //结算表状态异常
#define   ERR_SPDB_STLE_BCH_NOT_EXIST            200307             //结算批次记录不存在
#define   ERR_SPDB_STLE_BCH_STATUS_ERROR         200308             //结算批次状态异常
#define   ERR_STLE_STAT_NOT_ALLOW_SETTLE         200309             //结算单当前状态不允许结算
#define   ERR_BCH_STAT_NOT_ALLOW_REUPLOAD        200310             //结算批次当前状态不允许重新上传
#define   ERR_SPDB_CHECK_SIGN_FAIL               200311             //浦发返回报文验签失败
#define   ERR_SPDB_ACCT_NO_STATUS_INVALID        200312             //资金账户状态非法
#define   ERR_SPDB_PAY_TNL_STATUS_INVALID        200313             //代付通道状态非法
#define   ERR_BANKINSCODE_BRANCHNAME_EMPTY       200314             //开户行行号和名称同时为空

#define   ERR_REQ_SPDB_FAIL                      200399             //请求浦发前置机失败



#endif

