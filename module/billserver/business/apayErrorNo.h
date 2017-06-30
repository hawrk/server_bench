#ifndef _ERROR_NO_H_
#define _ERROR_NO_H_

#define   SYSTEM_ERR                200001             //系统错误
#define   QRY_DB_ERR               200002             //数据库查询错误
#define   INSERT_DB_ERR             200002             //数据库插入错误
#define   UPDATE_DB_ERR             200003             //数据库更新错误

#define   ERR_CALL_ID_SERVER          200101           //调用id_server错误
#define   ERR_CALL_APAY_SERVER        200102           //调用agentpay_server错误

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
#define   ERR_APAY_BILL_STATUS        200225             //当前对账状态不允许对账
#define   ERR_APAY_WEBANK_BILL_EMPTY  200226             //微众对账单为空
#define   ERR_APAY_NOT_BILL_DATA      200227             //微该天无交易,无法对账

#endif

