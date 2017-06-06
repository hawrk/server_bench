#ifndef __SPP_QUERY_1_H
#define __SPP_QUERY_1_H


#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	char user[32];
	char pwd[32];
	char host[32];
	char dbname[32];

} T_STSYBASE_IN_PARAM;

//定义库表中的SQL字段


//GetPayBillData
typedef struct
{
	char mch_id[64];
	int sum_shop_amount;
	int sum_channel_profit;
	int sum_service_profit;
	int sum_payment_profit;
	int sum_bm_profit;
	int sum_total_commission;
	int sum_total_fee;
	int count;
} stGetPayBillData_Resp;

//GetRefundBillData
typedef struct
{
	char mch_id[64];
	int sum_shop_amount;
	int sum_channel_profit;
	int sum_service_profit;
	int sum_payment_profit;
	int sum_bm_profit;
	int sum_total_commission;
	int sum_refund_fee;
	int count;
} stGetRefundBillData_Resp;

//GetChannelBillData
typedef struct
{
	char channel_id[64];
	int sum_channel_profit;
	
} stGetChannelBillData_Resp;

//GetWXOverFlowData
typedef struct
{
	char pay_time[32];
	char transaction_id[64];
	char order_no[64];
	char trade_type[32];
	char order_status[32];
	char refund_id[64];
	char refund_no[64];

} stGetWXOverFlowData_Resp;

//GetAliOverFlowData
typedef struct
{
	char transaction_id[64];
	char order_no[64];
	int  order_status;
	unsigned int pay_time;
	char refund_no[64];

} stGetAliOverFlowData_Resp;


/**
	操作函数:
	i_cmd: 0查询	1插入	2删除	3更新
 */
extern int  spp_bill_Insert_Update_Del( int i_cmd, T_STSYBASE_IN_PARAM* in_param,char* str_sql,int* _count);

extern int  spp_bill_Transaction(int i_cmd,T_STSYBASE_IN_PARAM* in_param,char* wxdel_sql,char* alidel_sql,char* wx_channdel_sql,char* ali_channdel_sql,int* _count);

extern int  spp_bill_GetPayBillData( int i_cmd, T_STSYBASE_IN_PARAM* in_param,stGetPayBillData_Resp *pT_SPP_BILL,char* str_sql,char* count_sql,int* _count);

extern int  spp_bill_GetRefundBillData( int i_cmd, T_STSYBASE_IN_PARAM* in_param,stGetRefundBillData_Resp *pT_SPP_BILL,char* str_sql,char* count_sql,int* _count);

extern int  spp_bill_GetChannelBillData( int i_cmd, T_STSYBASE_IN_PARAM* in_param,stGetChannelBillData_Resp *pT_SPP_BILL,char* str_sql,char* count_sql,int* _count);

extern int  spp_bill_GetAliOverFlowData( int i_cmd, T_STSYBASE_IN_PARAM* in_param,stGetAliOverFlowData_Resp *pT_SPP_BILL,char* str_sql,char* count_sql,int* _count);

extern int  spp_bill_GetWXOverFlowData( int i_cmd, T_STSYBASE_IN_PARAM* in_param,stGetWXOverFlowData_Resp* pT_SPP_BILL,char* str_sql,int count);

extern void spp_query_wx_list_init( char* user,char* pwd,char* host );
//取记录数
extern int spp_bill_getrecord_count(int i_cmd, T_STSYBASE_IN_PARAM* in_param,char* count_sql,int* _count);
//extern void spp_query_1_log( int i_cmd, const T_SPP_QUERY_WX *pt_spp_query_1 );

#ifdef __cplusplus
}
#endif


#endif
