#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sppincl.h"
#include "Singleton.h"
#include "CCommonTaskManager.h"
#include "clog.h"
#include "../Base/Comm/business/order_protocol.h"
#include "CSpeedPosConfig.h"

#include "CCreateRouteBill.h"
#include "CDownLoadCheckBill.h"
#include "CDownLoadSettleFile.h"
#include "CBillCheckTask.h"
#include "CRouteBillAbnormalQuery.h"
#include "CRouteBillSummaryQuery.h"
#include "CCreateMchBill.h"
#include "CMchBillQuery.h"
#include "CRouteSettleQuery.h"
#include "CLiquidationTask.h"

// 公共服务接口
#define CMD_CREATE_ROUTE_BILL    9010
#define CMD_DOWNLOAD_CHECK_BILL  9020
#define CMD_DOWNLOAD_STEELE      9030
#define CMD_BILL_CHECK_TASK      9040
#define CMD_BILL_SUMMARY_QUERY   9050
#define CMD_BILL_ABNORMAL_QUERY  9060
#define CMD_CREATE_MCH_BILL      9070
#define CMD_MCH_BILL_QUERY       9080
#define CMD_SETTLE_QUERY         9090
#define CMD_LIQUIDATION          9100


void RegTask()
{
	BEGIN_LOG(__func__);
	
	REGISTER_TASK(ORDER_VER_1, CMD_CREATE_ROUTE_BILL, CCreateRouteBill);  //生成本地对账记录
	REGISTER_TASK(ORDER_VER_1, CMD_DOWNLOAD_CHECK_BILL, CDownLoadCheckBill);  //下载通道对账单
	REGISTER_TASK(ORDER_VER_1, CMD_DOWNLOAD_STEELE, CDownLoadSettleFile);  //下载通道结算单
	REGISTER_TASK(ORDER_VER_1, CMD_BILL_CHECK_TASK, CBillCheckTask);  //对账执行
	REGISTER_TASK(ORDER_VER_1, CMD_BILL_SUMMARY_QUERY, CRouteBillSummaryQuery);  //对账汇总查询
	REGISTER_TASK(ORDER_VER_1, CMD_BILL_ABNORMAL_QUERY, CRouteBillAbnormalQuery);  //对账差错查询
	REGISTER_TASK(ORDER_VER_1, CMD_CREATE_MCH_BILL, CCreateMchBill);  //生成商户账单
	REGISTER_TASK(ORDER_VER_1, CMD_MCH_BILL_QUERY, CMchBillQuery);  //商户账单查询
	REGISTER_TASK(ORDER_VER_1, CMD_SETTLE_QUERY, CRouteSettleQuery);  //清分 结果查询
	REGISTER_TASK(ORDER_VER_1, CMD_LIQUIDATION, CLiquidationTask);  //清分操作
}


/**
 * @brief 业务模块初始化插件接口（可选实现proxy,worker）
 * @param arg1 - 配置文件
 * @param arg2 - 服务器容器对象
 * @return 0 - 成功, 其它失败
 */
extern "C" int spp_handle_init(void* arg1, void* arg2)
{
    const char * etc  = (const char*)arg1;
    CServerBase* base = (CServerBase*)arg2;

    base->log_.LOG_P_PID(LOG_DEBUG, "spp_handle_init, config:%s, servertype:%d\n", etc, base->servertype());

	int iRet = 0;

	CSpeedPosConfig* pOrder_conf = Singleton<CSpeedPosConfig>::GetInstance();

    if (base->servertype() == SERVER_TYPE_WORKER)
    {

		RegTask();
        iRet = Singleton<CCommonTaskManager>::GetInstance()->Init();
        if (iRet != 0)
        {
            CERROR_LOG("CCommonTaskManager Init Failed.Ret[%d] Err[%s]",
                    iRet, Singleton<CCommonTaskManager>::GetInstance()->GetErrorMessage() );
            return -1;
        }		
		/* 业务自身初始化 */
		// ......


		fprintf(stderr, "load config from xml...\n");
		iRet = pOrder_conf->LoadConfig(etc);
		if (iRet != 0)
		{
		    CERROR_LOG("Bill LoadConfig Failed.Ret[%d] Err[%s]\n",
		            iRet, pOrder_conf->GetErrorMessage() );
		    return -1;
		}
		pOrder_conf->PrintConfig();

		pOrder_conf->InitServer();

		// 初始化clog
		fprintf(stderr, "init clog...\n");
		iRet = pOrder_conf->InitLog();
		if (iRet != 0)
		{
		    CERROR_LOG("c_log_init Failed.Ret[%d]\n", iRet );
		    return -1;
		}		
    }
    
    return 0;
}


/**
 * @brief 业务模块检查报文合法性与分包接口(proxy)
 * @param flow - 请求包标志
 * @param arg1 - 数据块对象
 * @param arg2 - 服务器容器对象
 * @return ==0  数据包还未完整接收,继续等待
 *         > 0  数据包已经接收完整, 返回包长度
 *         < 0  数据包非法, 连接异常, 将断开TCP连接
 */
extern "C" int spp_handle_input(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    //TConnExtInfo* extinfo = (TConnExtInfo*)blob->extdata;
    //CServerBase* base = (CServerBase*)arg2;

    //base->log_.LOG_P(LOG_DEBUG, "spp_handle_input, %d, %d, %s\n",
    //                 flow,
    //                 blob->len,
    //                 inet_ntoa(*(struct in_addr*)&extinfo->remoteip_));

    CDEBUG_LOG("spp_handle_input flow[%d] blob.len[%d] inet_ntoa[%s]\n",
            flow, 
            blob->len, 
            inet_ntoa(*(struct in_addr*)(&((TConnExtInfo*)blob->extdata)->remoteip_)));

	
    // 协议以\r\n结尾
    const CHAR* p = blob->data;
    while ( p - blob->data <= (int)(blob->len-1) )
    {
        if ( *p == '\r' )
        {
            if ( ((int)(p - blob->data) <= (int)(blob->len-2))
                    && (*(p+1) == '\n') )
            {
                return (int)(p-blob->data+2); //+len(\r\y)
            }
        }
        ++p;
    }

    //return blob->len;

	return 0;
}

/**
 * @brief 业务模块报文按worker组分发接口(proxy)
 * @param flow - 请求包标志
 * @param arg1 - 数据块对象
 * @param arg2 - 服务器容器对象
 * @return 处理该报文的worker组id
 */
extern "C" int spp_handle_route(unsigned flow, void* arg1, void* arg2)
{
    return 1;
}

/**
 * @brief 业务模块报文,worker组的处理接口(worker)
 * @param flow - 请求包标志
 * @param arg1 - 数据块对象
 * @param arg2 - 服务器容器对象
 * @return 0 - 成功,其它表示失败
 */
extern "C" int spp_handle_process(unsigned flow, void* arg1, void* arg2)
{
    blob_type   * blob    = (blob_type*)arg1;
    TConnExtInfo* extinfo = (TConnExtInfo*)blob->extdata;

    CServerBase* base  = (CServerBase*)arg2;
    CTCommu    * commu = (CTCommu*)blob->owner;

    base->log_.LOG_P_PID(LOG_DEBUG, "spp_handle_process, %d, %d, %s\n",
                         flow,
                         blob->len,
                         inet_ntoa(*(struct in_addr*)&extinfo->remoteip_));

    int iRet = 0;

    // 解析url文本协议
    NameValueMap mapInputParam;
    char szInput[blob->len];
    memcpy( szInput, blob->data, blob->len );
    szInput[ blob->len-2 ] = '&';
    szInput[ blob->len-1 ] = '\0';

    CDEBUG_LOG("spp_handle_process flow[%d] blob.len[%d] szInput[%s]\n", flow, blob->len, szInput );

    iRet = TextToMapEx( szInput, "&", mapInputParam );
    if ( iRet != 0 )
    {
        CERROR_LOG( "Parse Input Failed.Ret[%d] Input[%s]", iRet, szInput );
        return -1;
    }

	// 建立任务
	IUrlProtocolTask *piTask = Singleton<CCommonTaskManager>::GetInstance()->CreateTask( mapInputParam );
	if ( NULL == piTask )
	{
	  CERROR_LOG( "CreateTask Failed.Ret[%d] Err[%s]",
			  iRet, Singleton<CCommonTaskManager>::GetInstance()->GetErrorMessage() );
	  return -1;
	}
	CDEBUG_LOG("spp_handle_process GetInstance CreateTask ok." ); 

	// 执行任务
	blob_type bRetBlob;
	bRetBlob.len = 0;
	bRetBlob.data = NULL;
	piTask->SetRequest(szInput, strlen(szInput));
	CDEBUG_LOG("piTssk.GetRequest[%s] ok.", piTask->GetRequest() ); 
	// fprintf(stderr, "xxxxxxxxxxxxxxxxxxxxxxxxx tmp x:%s\n", piTask->GetRequest());
	iRet = piTask->Execute( mapInputParam, &(bRetBlob.data), bRetBlob.len );
	piTask->ClearRequest();

	// 记录
	piTask->LogProcess();
	if( strlen(piTask->GetLogMsg()) > 0 )
	{
	  CINFO_LOG( "%s", piTask->GetLogMsg() );
	}

	if( strlen(piTask->GetTicketOrderMsg()) > 0 )
	{
	  CTICKET_ORDER_CREATE_LOG( "%s", piTask->GetTicketOrderMsg() );
	}

	if( strlen(piTask->GetTicketPayMsg()) > 0 )
	{
	  CTICKET_ORDER_PAY_LOG( "%s", piTask->GetTicketPayMsg() );
	}

	if ( iRet < 0 )
	{
	  CERROR_LOG( "Task Execute Failed.Ret[%d] Err[%s]",
			  iRet, piTask->GetErrorMessage() );

	  CTRACE_LOG( "input:%s\n", szInput);
	}

	if ( bRetBlob.len <= 0 ||
		  NULL==bRetBlob.data )
	{
	  return -1;
	}

	/** 发送应答 */
	iRet = commu->sendto(flow, &bRetBlob, NULL);
	if(unlikely(iRet))
	  CERROR_LOG( "commu sendto error, %d", iRet );

	free(bRetBlob.data);

    return 0;
}


/**
 * @brief 业务服务终止接口函数(proxy/worker)
 * @param arg1 - 保留
 * @param arg2 - 服务器容器对象
 * @return 0 - 成功,其它表示失败
 */
extern "C" void spp_handle_fini(void* arg1, void* arg2)
{
    CServerBase* base = (CServerBase*)arg2;
    base->log_.LOG_P(LOG_DEBUG, "spp_handle_fini\n");

    if ( base->servertype() == SERVER_TYPE_WORKER )
    {
        //CSyncFrame::Instance()->Destroy();
    }
}

#if 0
/**
 * @brief 提取模调上报信息的回调函数
 * @param flow - 请求包标志
 * @param arg1 - 数据块对象
 * @param arg2 - 上报实例
 * @return 0-成功, >0 无需上报, <0 失败上报异常
 */
extern "C" int spp_handle_report(unsigned flow, void* arg1, void* arg2)
{
	blob_type   * blob    = (blob_type*)arg1;
	CReport     * rpt     = (CReport *)arg2;

	char *pMsg = blob->data;
	int len = blob->len;
	
	uint32_t cmd = 0;
	int ret = 0;
	
	rpt->set_cmd(cmd);
	rpt->set_result(ret);
	
	return 0;
}
#endif
