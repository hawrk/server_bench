/*
 * order_spp_main.cpp
 * 集成到SPP框架的入口
 * Created on: 2010-5-27
 * Author: rogeryang
 */

#include "sppincl.h"
#include "Singleton.h"
#include "CSpeedPosConfig.h"
#include "CCommonTaskManager.h"
#include "../../Base/Comm/business/order_protocol.h"
#include "dc/dcreporter.h"
#ifdef _SYBASE

#else
#include "CProcPushSysBillTask.h"
#endif
//#include "CProcPushWxApiBillTask.h"
//#include "CProcPushAliApiBillTask.h"
#include "CProcBillContrastTask.h"
#include "speed_bill_protocol.h"
#include "CProcSettleCallback.h"
#include "CBillDownLoadManage.h"
#include "CBillBatchManage.h"
#include "CBillSummaryQry.h"
#include "CBillAbnormalQry.h"
#include "CSettleQryTask.h"

#include "CAgentPayBillDealTask.h"
#include "CAgentPayAbnormalQryTask.h"

#include "CSettleAcctinfoModifyTask.h"
#include "CAgentPayBillLogQryTask.h"

//base::comm::DCReporter  g_DCReporter;

// 每个Worker进程全局的OrderServer实例
#include "CSpeedPosServer.h"
CSpeedPosServer g_cOrderServer;

#include "log/clog.h"
#include <arpa/inet.h>

void RegTask()
{
	BEGIN_LOG(__func__);
#ifdef _SYBASE

#else
	REGISTER_TASK(ORDER_VER_1, CMD_SPEEDPOS_PSUH_SYS_BILL, CProcPushSysBillTask);  //对账单下载
#endif

	//REGISTER_TASK(ORDER_VER_1, CMD_SPEEDPOS_PSUH_WX_API_BILL, CProcPushWxApiBillTask); //微信对账单下载（暂时废弃）

	//REGISTER_TASK(ORDER_VER_1, CMD_SPEEDPOS_PSUH_ALI_API_BILL, CProcPushAliApiBillTask); //支付宝对账单下载（暂时废弃）

	REGISTER_TASK(ORDER_VER_1, CMD_SPEEDPOS_CONTRAST_BILL, CProcBillContrastTask);   //对账操作

	REGISTER_TASK(ORDER_VER_1,CMD_SPEEDPOS_SETTLE_CALLBACK,CProcSettleCallback);  //清分结果回调

	REGISTER_TASK(ORDER_VER_1,CMD_SPEEDPOS_BILL_DOWNLOAD,CBillDownLoadManage);  //对账单下载查询

	REGISTER_TASK(ORDER_VER_1,CMD_SPEEDPOS_BILL_BATCH,CBillBatchManage);

	REGISTER_TASK(ORDER_VER_1,CMD_SPEEDPOS_BILL_SUMMARY,CBillSummaryQry); //对账总览

	REGISTER_TASK(ORDER_VER_1,CMD_SPEEDPOS_BILL_ABNORMAL,CBillAbnormalQry); //对账总览

	REGISTER_TASK(ORDER_VER_1,CMD_SPEEDPOS_BILL_SETTLE_QRY,CSettleQryTask);  //结算查询
	
	REGISTER_TASK(ORDER_VER_1,CMD_SPEEDPOS_APAY_BILL_DEAL,CAgentPayBillDealTask);  //代付对账处理

	REGISTER_TASK(ORDER_VER_1,CMD_SPEEDPOS_APAY_ABNOR_BILL_QRY,CAgentPayAbnormalQryTask);  //代付对账处理	

	REGISTER_TASK(ORDER_VER_1,CMD_SPEEDPOS_SETTLE_ACCTINFO_MODIFY,CSettleAcctinfoModifyTask);  //结算账户信息编辑

    REGISTER_TASK(ORDER_VER_1,CMD_SPEEDPOS_APAY_BILL_LOG_QRY,CAPayBillLogQryTask);  //代付对账记录查询
}

// 初始化方法（可选实现）,返回0成功，非0失败
extern "C" int spp_handle_init(void* arg1, void* arg2)
{
    BEGIN_LOG(__func__);

    int iRet = 0;

    // 插件自身的配置文件
    const char* etc = (const char*)arg1;
    // 服务器容器对象
    CServerBase* base = (CServerBase*)arg2;
    // 建立一个统计项, 统计策略为累加
    // base->stat_.init_statobj(MODULE_PROC_NUM, STAT_TYPE_SUM);

    CDEBUG_LOG("spp_handle_init, %s, %d\n", etc, base->servertype());

	CSpeedPosConfig* pOrder_conf = Singleton<CSpeedPosConfig>::GetInstance();

    switch (base->servertype())
    {
        case SERVER_TYPE_CTRL:
        case SERVER_TYPE_PROXY:
            return 0;
        case SERVER_TYPE_WORKER:    // 加载配置
            fprintf(stderr, "load config from xml...\n");
            iRet = pOrder_conf->LoadConfig(etc);
            if (iRet != 0)
            {
                CERROR_LOG("Bill LoadConfig Failed.Ret[%d] Err[%s]\n",
                        iRet, pOrder_conf->GetErrorMessage() );
                return -1;
            }
            pOrder_conf->PrintConfig();

            // 初始化clog
            fprintf(stderr, "init clog...\n");
            iRet = pOrder_conf->InitLog();
            if (iRet != 0)
            {
                CERROR_LOG("c_log_init Failed.Ret[%d]\n", iRet );
                return -1;
            }

            // 初始化全局OrderServer
            fprintf(stderr, "init global BillServer...\n");
			iRet = pOrder_conf->InitOrderServer(g_cOrderServer);
            if (iRet != 0)
            {
                CERROR_LOG("BillServer init Failed.Ret[%d] Err[%s]\n", iRet, g_cOrderServer.GetErrorMessage());
                return -1;
            }

		   // 初始化Server
		   fprintf(stderr, "init Server...\n");
		   iRet = pOrder_conf->InitServer();
		   if (iRet != 0)
		   {
			   CERROR_LOG("Server init Failed.Ret[%d]\n", iRet);
			   return -1;
		   }


           /* fprintf(stderr, "init dc reporter...\n");
            iRet = pOrder_conf->InitDCReporter(g_DCReporter);
            if (iRet != 0)
            {
                CERROR_LOG("Init DC Reporter Failed.Ret[%d]\n", iRet );
                return -1;
            }*/

            // 任务分派器
            fprintf(stderr, "init task manager...\n");
            RegTask();
            iRet = Singleton<CCommonTaskManager>::GetInstance()->Init();
            if (iRet != 0)
            {
                CERROR_LOG("CCommonTaskManager Init Failed.Ret[%d] Err[%s]",
                        iRet, Singleton<CCommonTaskManager>::GetInstance()->GetErrorMessage() );
                return -1;
            }

            break;
        default:
            CERROR_LOG("Server Init Error! Unknown server type: [%d]\n", base->servertype());
            return -100;
    }

    fprintf(stderr, "init succ.\n");
    return 0;
}

// 数据接收（必须实现）,返回正数表示数据已经接收完整且该值表示数据包的长度，
// 0值表示数据包还未接收完整，负数表示出错
extern "C" int spp_handle_input(unsigned flow, void* arg1, void* arg2)
{
    BEGIN_LOG(__func__);
    // 数据块对象，结构请参考tcommu.h
    blob_type* blob = (blob_type*)arg1;
    // extinfo有扩展信息
    // TConnExtInfo* extinfo = (TConnExtInfo*)blob->extdata;

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

    return 0;
}

// 数据处理（必须实现）
// flow: 请求包标志
// arg1: 数据块对象
// arg2: 服务器容器对象
// 返回0表示成功，非0失败
extern "C" int spp_handle_process(unsigned flow, void* arg1, void* arg2)
{
    BEGIN_LOG(__func__);
    // 数据块对象，结构请参考tcommu.h
    blob_type* blob = (blob_type*)arg1;
    // 数据来源的通讯组件对象
    CTCommu* commu = (CTCommu*)blob->owner;

    // 统计项加1
    //base->stat_.step0(MODULE_PROC_NUM, 1);

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
    CDEBUG_LOG("spp_handle_process TextToMapEx ok." ); 


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

//路由选择（可选实现）
//flow:请求包标志
//arg1:数据块对象
//arg2:服务器容器对象
//返回值表示worker的groupid
//extern "C" int spp_handle_route(unsigned flow, void* arg1, void* arg2)
//
//{
//
//    //服务器容器对象
//    CServerBase* base = (CServerBase*)arg2;
//
//    base->log_.LOG_P(LOG_DEBUG, "spp_handle_route, %d\n", flow);   
//    int RandomNumber;
//
//    srand((unsigned)time(NULL));//为rand()函数生成不同的随机种子
//    RandomNumber = rand()%20;//生成20以内的随机数
//    if ( RandomNumber == 0 )
//    {
//        RandomNumber += 1;
//    }
//    printf ( "RandomNumber====%d.\n", RandomNumber );
//
//    return RandomNumber;
//
//}
//
