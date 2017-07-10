/**
 *  @file mt_version.h
 *  @info 微线程版本更新与记录信息文件
 *  @time 20131018
 *  ------------------------------------------------------------------------
 *  @brief v0.2.0 - 支持UDP按session模式映射上下文
 *         v0.2.1 - 支持TCP的sendrcv接口
 */

#ifndef _MT_VERSION_EX__
#define _MT_VERSION_EX__

namespace NS_MICRO_THREAD {

__attribute__((weak)) char IMT_VERSION[64] = "mt version: 0.2.2";

} // NAMESPACE NS_MICRO_THREAD

#endif

