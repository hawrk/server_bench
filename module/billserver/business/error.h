#ifndef _WEB_GATE_ERROR_H_
#define _WEB_GATE_ERROR_H_
#include <map>
#include <string>
#include <ostream>
using std::map;
using std::string;

const int SUCCESS_RESULT = 0; // 成功





/**
 * webgate error
 */
const int ERR_PARAM_INVALID = 10001; //

const int ERR_PASE_RETRUN_DATA = 10002; //解析返回的数据错误

const int ERR_BUSINESS_NO_EXIST = 10003; //此商户不存在

const int ERR_BUSINESS_NO_AUDITED = 10004; //此商户未审核成功

const int ERR_SIGN_CHECK = 10005; //校验SIGN失败

const int ERR_PAYMENT_TYPE_NO_EXIST = 10006; //无此支付方式

const int ERR_BUSINESS_NOT_OPEN_PATMENT_METHOD = 10007; // 此商户未开通此支付方式

const int ERR_BUSINESS_NOT_ENABLED_PATMENT_METHOD = 10008; // 此商户未启用此支付方式

const int ERR_PAYMENT_AMOUNT_TOO_SMALL = 10009; // 支付金额过小

const int ERR_PAYMENT_AMOUNT_TOO_BIG = 10010; // 支付金额过大

const int ERR_WX_PASE_RETRUN_DATA = 10011; //解析微信返回的数据错误

const int ERR_WX_SIGN_CHECK = 10012; //校验微信SIGN失败

const int ERR_GET_BUSINESS_DAY_TRADE_FLOW_VALUE = 10013; //获取日交易量失败

const int ERR_BUSINESS_DAY_PAYMENT_AMOUNT_HAS_REACH_LIMIT = 10014; //商户日交易额已达限制

const int ERR_PRODUCE_DUPLICATE_ORDER_NO = 10015; //产生重复订单号
/**
*  wx 11001 begin
**/
const int ERR_CALL_WX_UNIFIEDORDER_REQ  = 11001; //调用微信下单请求

const int ERR_CALL_WX_NOTIFY_REQ = 11002; //微信支付回调错误

const int ERR_CALL_WX_ORDERQUERY_REQ = 11003; //微信订单查询错误

const int ERR_CALL_WX_CURL_REQ = 11004;//
/* spp server error */
const int ERR_THIRT_ORDERNO_ALREADY_EXISTS = 12001; //第三方订单号已存在

const int ERR_CREATE_ORDER = 12002; //创建订单失败

/* bill settle error */
const int ERR_BILL_SETTLE_NOT_EXIST = 13001; //结算单不存在
const int ERR_BILL_SETTLE_STATUS = 13002; //结算状态有误

const int ERR_ORDER_TOTALNET_IS_NOT_ENOUGH_TO_REFUND = -650;

const int ERR_SHOP_DAY_AMOUT_IS_NOT_ENOUGH_TO_REFUND = -700;



/** call curl error*/
const int ERR_CREATE_XMLNode = -1001;

const int ERR_CURL_EASY_INIT = -1002;

const int ERR_ALI_PAY_RSA_VERIFY = -1003;

const int ERR_CACHE_NO_FUND_KEY = -1004;

const int ERR_ALI_PAY_TOTAL_FEE_IS_NULL = -1005;

const int ERR_THIRD_PARTY_NOT_RECIVE_NOTIFY = -1006;

const int ERR_PARAM_VALUE_INVALID = -1007;

const int ERR_ALI_PAY_REFUND_ERR_MSG = -1008;

const int ERR_ALI_PAY_UNIFIEDORDER_ERR_MSG = -1009;

const int ERR_ALI_PAY_TRADE_NO_SUCCESS = -1010;

const int ERR_LACK_NECESSARY_PARAM = -1011;

const int ERR_ORDER_NO_MAPPING_EXIST = -1012;

const int ERR_CALL_WX_REFUND_REQ = -1014; //调用微信下单请求

const int ERR_REFUNDNO_NO_EXIST = -1015;

const int ERR_CALL_WX_REFUNDQUERY_REQ = -1016; //调用微信退款查询

const int ERR_PUSH_RABBIT_MQ = -1017; //

const int ERR_CREATE_ORDER_NO = -1018; //

const int ERR_ALI_PAY_ORDER_NO_NO_EXIST = -1019;

const int ERR_BILL_CONTRAST_MODE = -2001; //



const int ERR_WXPAY_NOT_RECONCLIED = -2010; //微信对账不平

const int ERR_ALIPAY_NOT_RECONCLIED = -2020; //支付宝对账不平

const int ERR_NOTIFY_SETTLE_FAILED = -2030;  //通知结算服务失败

//sys error
const int ERR_FILE_DIR_CREATE_FAILED   = -9900;    //创建目录失败
const int ERR_CONFIG_NOT_FOUND         = -9901;    //找不到配置信息
const int ERR_BILL_FILE_EXIST          = -9902;    //结算文件已存在
const int ERR_DETAIL_FILE_NOT_FOUND    = -9903;    //下载文件找不到
const int ERR_UPDATE_SETTLE_STATUS     = -9904;    //更新结算记录失败
const int ERR_UPDATE_BILL_STATUS       = -9905;    //更新对账记录失败
const int ERR_DOWNLOAD_FILE            = -9906;    //对账文件下载失败
const int ERR_QUERY_RECORD_ERR         = -9907;    //查询记录失败
const int ERR_GET_FAILBILL_ERR         = -9908;    //获取未打款记录失败
const int ERR_BANK_TYPE                = -9909;    //不支持此银行请求
const int ERR_DOWNLOAD_FILE_EXIST      = -9910;    //对账文件已存在
const int ERR_DB_UPDATE                = -9911;    //数据更新失败
const int ERR_BILL_BATCH_EXIST         = -9912;    //正在对账中
const int ERR_DB_INSERT                = -9913;    //数据写入失败

#ifdef  _KOREA_VER
class ErrParamMap : public std::map<int, std::string>
{
public:
	ErrParamMap()
	{
		this->insert(std::make_pair(ERR_PARAM_INVALID, "파라미터 틀림!"));
		this->insert(std::make_pair(ERR_PASE_RETRUN_DATA, "데이터 분석 오류!"));
		this->insert(std::make_pair(ERR_WX_PASE_RETRUN_DATA, "위체에서 돌려 받은 데이터 분석 오류!"));
		this->insert(std::make_pair(ERR_SIGN_CHECK, "SIGN검증 실패!"));
		this->insert(std::make_pair(ERR_WX_SIGN_CHECK, "위쳇 SIGN검증 실패!"));

		this->insert(std::make_pair(ERR_BUSINESS_NO_EXIST, "존재하지 않는 가맹점!"));
		this->insert(std::make_pair(ERR_BUSINESS_NO_AUDITED, "가맹점 심사 불통과!"));

		this->insert(std::make_pair(ERR_PAYMENT_TYPE_NO_EXIST, "지지되지 않는 지급 방식입니다!"));
		this->insert(std::make_pair(ERR_BUSINESS_NOT_OPEN_PATMENT_METHOD, "가맹점에서 해당 지급방식은 아직 활성화 시키지 않았습니다!"));
		this->insert(std::make_pair(ERR_BUSINESS_NOT_ENABLED_PATMENT_METHOD, "가맹점에서 해당 지급방식은 아직 활성화 시키지 않았습니다!"));
		this->insert(std::make_pair(ERR_PAYMENT_AMOUNT_TOO_SMALL, "최소 거래 금액에 도달하지 못하였습니다!"));
		this->insert(std::make_pair(ERR_PAYMENT_AMOUNT_TOO_BIG, "최대 거래 금액을 초과 하였습니다!"));
		this->insert(std::make_pair(ERR_GET_BUSINESS_DAY_TRADE_FLOW_VALUE, "당일 거래기록 취득 실패!"));
		this->insert(std::make_pair(ERR_BUSINESS_DAY_PAYMENT_AMOUNT_HAS_REACH_LIMIT, "당일 가맹점 최대 거래 금액 한도에 도달 하였습니다!"));

		this->insert(std::make_pair(ERR_PRODUCE_DUPLICATE_ORDER_NO, "이미 존재한 제3자 주문 홀수 번호입니다!"));
		//wx req
		this->insert(std::make_pair(ERR_CALL_WX_UNIFIEDORDER_REQ, "위쳇 주문 요청 실패!"));
		this->insert(std::make_pair(ERR_CALL_WX_NOTIFY_REQ, "위쳇 콜백 실패!"));
		this->insert(std::make_pair(ERR_CALL_WX_ORDERQUERY_REQ, "위쳇 주문 조회 실패!"));

		this->insert(std::make_pair(ERR_CALL_WX_CURL_REQ, "요청시간 초과하였습니다, 잠시후 다시 시도 하세요!"));

		//spp req
		this->insert(std::make_pair(ERR_THIRT_ORDERNO_ALREADY_EXISTS, "이미 존재한 제3자 주문 홀수 번호입니다!"));
		this->insert(std::make_pair(ERR_CREATE_ORDER, "주문 생성 실패!"));

		//curl
		this->insert(std::make_pair(ERR_CREATE_XMLNode, "XML초기화 실패!"));
		this->insert(std::make_pair(ERR_CURL_EASY_INIT, "CURL초기화 실패!"));

		this->insert(std::make_pair(ERR_ALI_PAY_RSA_VERIFY, "알리페이 서명 검증 실패!"));

		this->insert(std::make_pair(ERR_ALI_PAY_TRADE_NO_SUCCESS, "알리페이 지급 실패!"));

		this->insert(std::make_pair(ERR_CACHE_NO_FUND_KEY, "캐시 키가 존재하지 않습니다!"));

		this->insert(std::make_pair(ERR_ALI_PAY_ORDER_NO_NO_EXIST, "홀수 번호가 존재 하지 않습니다!"));

		this->insert(std::make_pair(ERR_ALI_PAY_TOTAL_FEE_IS_NULL, "알리페이 주문 금액은 비여있습니다!"));

		this->insert(std::make_pair(ERR_THIRD_PARTY_NOT_RECIVE_NOTIFY, "제3자에서 지급 콜백을 받지 못하였습니다!"));

		this->insert(std::make_pair(ERR_PARAM_VALUE_INVALID, "뮤효한 파라미터 입니다!"));

		this->insert(std::make_pair(ERR_ALI_PAY_REFUND_ERR_MSG, "알리페이 환불 반환 오류!"));

		this->insert(std::make_pair(ERR_ALI_PAY_UNIFIEDORDER_ERR_MSG, "알리페이 주문 반환 오류!"));

		this->insert(std::make_pair(ERR_LACK_NECESSARY_PARAM, "필수적인 파라미터가 부족합니다!"));

		this->insert(std::make_pair(ERR_ORDER_NO_MAPPING_EXIST, "주문 매핑이 존재하지 않습니다!"));

		this->insert(std::make_pair(ERR_CALL_WX_REFUND_REQ, "위쳇 환불 요청 실패!"));

		this->insert(std::make_pair(ERR_REFUNDNO_NO_EXIST, "존재하지 않는 환불 홀수 번호 입니다!"));

		this->insert(std::make_pair(ERR_CALL_WX_REFUNDQUERY_REQ, "환불 조회 실패!"));

		this->insert(std::make_pair(ERR_PUSH_RABBIT_MQ, "MQ입력 실패!"));

		this->insert(std::make_pair(ERR_SHOP_DAY_AMOUT_IS_NOT_ENOUGH_TO_REFUND, "가맹점 당일의 거래금액은 환불 한도에 도달하지 못하였습니다!"));

		this->insert(std::make_pair(ERR_ORDER_TOTALNET_IS_NOT_ENOUGH_TO_REFUND, "주문 순액은 환불 한도에 도달하지 못하였습니다!"));

		this->insert(std::make_pair(ERR_CREATE_ORDER_NO, "주문 번호 생성 실패!"));

		this->insert(std::make_pair(ERR_WXPAY_NOT_RECONCLIED, "위쳇 계산서 일치하지 않음!"));

		this->insert(std::make_pair(ERR_ALIPAY_NOT_RECONCLIED, "알리페이 계산서 일치하지 않음!"));

		this->insert(std::make_pair(ERR_BILL_CONTRAST_MODE, "계산서 대조 방식이 틀렸습니다!"));

		this->insert(std::make_pair(ERR_NOTIFY_SETTLE_FAILED,"청산 서비스 통지 실패！"));

		this->insert(std::make_pair(ERR_FILE_DIR_CREATE_FAILED,"디렉토리를 만들 수 없습니다"));

		this->insert(std::make_pair(ERR_CONFIG_NOT_FOUND,"구성 정보를 찾을 수 없습니다"));

		this->insert(std::make_pair(ERR_BILL_FILE_EXIST,"파일이 이미 존재합니다"));
	}

	virtual ~ErrParamMap()
	{

	}

private:

};
#else
class ErrParamMap : public std::map<int, std::string>
{
public:
	ErrParamMap()
	{
		this->insert(std::make_pair(ERR_PARAM_INVALID, "参数错误!"));
		this->insert(std::make_pair(ERR_PASE_RETRUN_DATA, "解析数据错误!"));
		this->insert(std::make_pair(ERR_WX_PASE_RETRUN_DATA, "解析微信返回的数据错误!"));
		this->insert(std::make_pair(ERR_SIGN_CHECK, "校验SIGN失败!"));
		this->insert(std::make_pair(ERR_WX_SIGN_CHECK, "校验微信SIGN失败!"));
		
		this->insert(std::make_pair(ERR_BUSINESS_NO_EXIST, "商户不存在!"));
		this->insert(std::make_pair(ERR_BUSINESS_NO_AUDITED, "商户未审核成功!"));
		
		this->insert(std::make_pair(ERR_PAYMENT_TYPE_NO_EXIST, "未支持此支付方式!"));
		this->insert(std::make_pair(ERR_BUSINESS_NOT_OPEN_PATMENT_METHOD, "此商户未开通此支付方式!"));
		this->insert(std::make_pair(ERR_BUSINESS_NOT_ENABLED_PATMENT_METHOD, "此商户未启用此支付方式!"));
		this->insert(std::make_pair(ERR_PAYMENT_AMOUNT_TOO_SMALL, "支付金额过小!"));
		this->insert(std::make_pair(ERR_PAYMENT_AMOUNT_TOO_BIG, "支付金额过大!"));
		this->insert(std::make_pair(ERR_GET_BUSINESS_DAY_TRADE_FLOW_VALUE, "获取日交易量失败!"));
		this->insert(std::make_pair(ERR_BUSINESS_DAY_PAYMENT_AMOUNT_HAS_REACH_LIMIT, "商户日交易额已达限制!"));

		this->insert(std::make_pair(ERR_PRODUCE_DUPLICATE_ORDER_NO, "第三方订单号已存在!"));
		//wx req
		this->insert(std::make_pair(ERR_CALL_WX_UNIFIEDORDER_REQ, "微信下单请求失败!"));
		this->insert(std::make_pair(ERR_CALL_WX_NOTIFY_REQ, "微信回调失败!"));
		this->insert(std::make_pair(ERR_CALL_WX_ORDERQUERY_REQ, "微信订单查询失败!"));

		this->insert(std::make_pair(ERR_CALL_WX_CURL_REQ, "请求超时,请稍后在试!"));

		//spp req
		this->insert(std::make_pair(ERR_THIRT_ORDERNO_ALREADY_EXISTS, "第三方订单号已存在!"));
		this->insert(std::make_pair(ERR_CREATE_ORDER, "创建订单失败!"));

		//curl
		this->insert(std::make_pair(ERR_CREATE_XMLNode, "XML初始化失败!"));
		this->insert(std::make_pair(ERR_CURL_EASY_INIT, "CURL初始化失败!"));

		this->insert(std::make_pair(ERR_ALI_PAY_RSA_VERIFY, "支付宝验签失败!"));

		this->insert(std::make_pair(ERR_ALI_PAY_TRADE_NO_SUCCESS, "支付宝支付失败!"));

		this->insert(std::make_pair(ERR_CACHE_NO_FUND_KEY, "缓存键不在存在!"));

		this->insert(std::make_pair(ERR_ALI_PAY_ORDER_NO_NO_EXIST, "订单号不存在!"));

		this->insert(std::make_pair(ERR_ALI_PAY_TOTAL_FEE_IS_NULL, "支付宝订单金额为空!"));

		this->insert(std::make_pair(ERR_THIRD_PARTY_NOT_RECIVE_NOTIFY, "第三方未收到支付回调!"));

		this->insert(std::make_pair(ERR_PARAM_VALUE_INVALID, "参数值无效!"));

		this->insert(std::make_pair(ERR_ALI_PAY_REFUND_ERR_MSG, "支付宝退款返回出错!"));

		this->insert(std::make_pair(ERR_ALI_PAY_UNIFIEDORDER_ERR_MSG, "支付宝下单返回出错!"));

		this->insert(std::make_pair(ERR_LACK_NECESSARY_PARAM, "缺少必要的参数!"));

		this->insert(std::make_pair(ERR_ORDER_NO_MAPPING_EXIST, "订单映射不存在!"));

		this->insert(std::make_pair(ERR_CALL_WX_REFUND_REQ, "微信退款请求失败!"));

		this->insert(std::make_pair(ERR_REFUNDNO_NO_EXIST, "退款单号不存在!"));

		this->insert(std::make_pair(ERR_CALL_WX_REFUNDQUERY_REQ, "退款查询失败!"));

		this->insert(std::make_pair(ERR_PUSH_RABBIT_MQ, "插入MQ失败!"));

		this->insert(std::make_pair(ERR_SHOP_DAY_AMOUT_IS_NOT_ENOUGH_TO_REFUND, "商户当天交易额不足以退款!"));

		this->insert(std::make_pair(ERR_ORDER_TOTALNET_IS_NOT_ENOUGH_TO_REFUND, "订单总净额不足以退款!"));

		this->insert(std::make_pair(ERR_CREATE_ORDER_NO, "创建订单号失败!"));

		this->insert(std::make_pair(ERR_WXPAY_NOT_RECONCLIED, "微信对账不平!"));

		this->insert(std::make_pair(ERR_ALIPAY_NOT_RECONCLIED, "支付宝对账不平!"));

		this->insert(std::make_pair(ERR_BILL_CONTRAST_MODE, "对账方式错误!"));

		this->insert(std::make_pair(ERR_NOTIFY_SETTLE_FAILED,"通知清算服务失败！"));

		this->insert(std::make_pair(ERR_FILE_DIR_CREATE_FAILED,"内部错误:创建目录失败！"));

		this->insert(std::make_pair(ERR_CONFIG_NOT_FOUND,"内部错误:找不到配置信息！"));

		this->insert(std::make_pair(ERR_BILL_FILE_EXIST,"结算文件已存在！"));

		this->insert(std::make_pair(ERR_DETAIL_FILE_NOT_FOUND,"下载文件不存在！"));

		this->insert(std::make_pair(ERR_UPDATE_SETTLE_STATUS,"内部错误:更新结算记录失败"));

		this->insert(std::make_pair(ERR_UPDATE_BILL_STATUS,"内部错误:更新对账记录失败"));

		this->insert(std::make_pair(ERR_DOWNLOAD_FILE,"内部错误:对账文件下载失败"));

		this->insert(std::make_pair(ERR_QUERY_RECORD_ERR,"内部错误:查询数据失败"));

		this->insert(std::make_pair(ERR_GET_FAILBILL_ERR,"获取未打款记录失败"));

		this->insert(std::make_pair(ERR_BANK_TYPE,"不支持此银行请求"));

		this->insert(std::make_pair(ERR_DOWNLOAD_FILE_EXIST,"对账文件已下载成功！"));

		this->insert(std::make_pair(ERR_DB_UPDATE,"数据更新失败！"));

		this->insert(std::make_pair(ERR_BILL_BATCH_EXIST,"对账记录已存在！"));

	}

	virtual ~ErrParamMap()
	{
		
	}

private:
	
};
#endif


const int BILL_CONTRAST_STEP_BEGIN_ING = 1;
const int BILL_CONTRAST_STEP_BEGIN_SFTP_DOWNLOAD = 2;
const int BILL_CONTRAST_STEP_BEGIN_LOAD_BILL_FLOW = 3;
const int BILL_CONTRAST_STEP_BEGIN_RECONCILIATION = 4;
const int BILL_CONTRAST_STEP_BEGIN_GENERATE_STATEMENT = 5;
const int BILL_CONTRAST_STEP_BEGIN_NOTIFY_INSTITUTION = 6;

#ifdef  _KOREA_VER
class StepMap : public std::map<int, std::string>
{
public:
	StepMap()
	{
		this->insert(std::make_pair(BILL_CONTRAST_STEP_BEGIN_ING, "계산서 대조 시작!"));
		this->insert(std::make_pair(BILL_CONTRAST_STEP_BEGIN_SFTP_DOWNLOAD, "SFTP다로운드 시작!"));
		this->insert(std::make_pair(BILL_CONTRAST_STEP_BEGIN_LOAD_BILL_FLOW, "금전 출납부를 DB로 기재하기 시작!"));
		this->insert(std::make_pair(BILL_CONTRAST_STEP_BEGIN_RECONCILIATION, "시스템 계산서 대조 집행!"));
		this->insert(std::make_pair(BILL_CONTRAST_STEP_BEGIN_GENERATE_STATEMENT, "계산서 생성하다!"));
		this->insert(std::make_pair(BILL_CONTRAST_STEP_BEGIN_NOTIFY_INSTITUTION, "기관에 통지하다!"));
	}
	virtual ~StepMap()
	{

	}
};
#else
class StepMap : public std::map<int, std::string>
{
public:
	StepMap()
	{
		this->insert(std::make_pair(BILL_CONTRAST_STEP_BEGIN_ING, "开始对账ING!"));
		this->insert(std::make_pair(BILL_CONTRAST_STEP_BEGIN_SFTP_DOWNLOAD, "开始SFTP下载!"));
		this->insert(std::make_pair(BILL_CONTRAST_STEP_BEGIN_LOAD_BILL_FLOW, "开始加载账单流水到DB!"));
		this->insert(std::make_pair(BILL_CONTRAST_STEP_BEGIN_RECONCILIATION, "执行系统对账!"));
		this->insert(std::make_pair(BILL_CONTRAST_STEP_BEGIN_GENERATE_STATEMENT, "生成入账单文件!"));
		this->insert(std::make_pair(BILL_CONTRAST_STEP_BEGIN_NOTIFY_INSTITUTION, "通知给机构!"));
	}
	virtual ~StepMap()
	{
		
	}
};
#endif
#endif

