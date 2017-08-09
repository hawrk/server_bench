#ifndef _ERROR_NO_H_
#define _ERROR_NO_H_

#define   SYSTEM_ERR                200001             //系统错误
#define   QRY_DB_ERR               200002             //数据库查询错误
#define   INSERT_DB_ERR             200003             //数据库插入错误
#define   UPDATE_DB_ERR             200004             //数据库更新错误
#define   ERR_RECORD_EXIST            200005             //数据库记录已存在
#define   ERR_PARSE_XML_FAIL          200006             //解析xml失败
#define   ERR_PARSE_JSON_FAIL         200007             //解析Json失败


#define   ERR_CHECK_BILL_EXIST        900001            //当日对账记录已生在
#define   ERR_DOWNLOAD_BILL           900002            //下载对账单失败
#define   ERR_DOWNLOAD_SETTLE         900003            //下载结算单失败
#define   ERR_QUERY_ORDER_STATE       900004            //查询本地订单状态失败
#define   ERR_CALL_AUTH_SERV          900005            //调用鉴权服务失败
#define   ERR_CALL_PAYGATE_SERV       900006            //调用网关服务失败
#define   ERR_GET_BANK_RESP           900007            //获取银行返回信息失败
#define   ERR_BILL_FILE_NOT_EXIST     900008            //对账文件不存在
#define   ERR_MCH_BILL_EXIST          900009            //商户对账单已生成
#define   ERR_CHECK_BILL_NOT_EXIST    900010            //未生成当日对账记录


#define   ERR_INVALID_PARAMS          200201             //关键入错错误
#define   ERR_MCH_STATUS              200202             //商户状态有误

#endif

