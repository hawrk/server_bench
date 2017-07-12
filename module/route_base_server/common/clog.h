#ifndef _C_LOG_H
#define _C_LOG_H
#include <sys/cdefs.h>
#include <sys/types.h>
#include <arpa/inet.h>

__BEGIN_DECLS
#undef __attr_cdecl__
/*
#define __attr_cdecl__ __attribute__((__cdecl__))
 */
#define __attr_cdecl__ /* */

#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
#define __builtin_expect(x, expected_value) (x)
#endif

#undef __attr_pure__
#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
#define __attr_pure__   /* */
#else
#define __attr_pure__   __attribute__((__pure__))
#endif

#undef __attr_nonnull__
#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
#define __attr_nonnull__(x)     /* */
#else
#define __attr_nonnull__(x) __attribute__((__nonnull__(x)))
#endif

#ifndef likely
#define likely(x)  __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

#define LOG_EMERG           0  /* system is unusable               */
#define LOG_ALERT           1  /* action must be taken immediately */
#define LOG_CRIT            2  /* critical conditions              */
#define LOG_ERROR           3  /* error conditions                 */
#define LOG_WARNING         4  /* warning conditions               */
#define LOG_NOTICE          5  /* normal but significant condition */
#define LOG_INFO            6  /* informational                    */
#define LOG_DEBUG           7  /* debug-level messages             */
#define LOG_TRACE           8  /* trace-level messages             */
#define LOG_ASYNC_BUSINESS  9  /* for async business messages      */
#define LOG_TICKET_ORDER_CREATE 10  /* for ticket agent create order      */
#define LOG_TICKET_ORDER_PAY    11  /* for ticket agent pay      */
#define LOG_TICKET_ORDER_TOKEN  12  /* for ticket agent refund      */
#define LOG_END             13 /* end log index*/

#define CDETAIL(level, fmt, args...) \
    c_write_log (level, "[%s][%d]%s: " \
            fmt "\n", \
            __FILE__, \
            __LINE__, \
            __FUNCTION__, \
##args)

#define CSIMPLY(level, fmt, args...) c_write_log(level, fmt "\n" , ##args)

#define BEGIN_LOG(func) c_write_slicing(LOG_INFO, func)

#define CERROR_LOG(fmt, args...)    CDETAIL(LOG_ERROR, fmt , ##args)
#define CCRIT_LOG(fmt, args...)    CDETAIL(LOG_CRIT, fmt , ##args)
#define CALERT_LOG(fmt, args...)    CDETAIL(LOG_ALERT, fmt , ##args)
#define CEMERG_LOG(fmt, args...)    CDETAIL(LOG_EMERG, fmt , ##args)

#define CWARN_LOG(fmt, args...)    CDETAIL(LOG_WARNING, fmt , ##args)
#define CNOTI_LOG(fmt, args...)    CDETAIL(LOG_NOTICE, fmt , ##args)
#define CINFO_LOG(fmt, args...)    CDETAIL(LOG_INFO, fmt , ##args)
// #define CWARN_LOG(fmt, args...)    CSIMPLY(LOG_WARNING, fmt , ##args)
// #define CNOTI_LOG(fmt, args...)    CSIMPLY(LOG_NOTICE, fmt , ##args)
// #define CINFO_LOG(fmt, args...)    CSIMPLY(LOG_INFO, fmt , ##args)


#define CTICKET_ORDER_CREATE_LOG(fmt, args...)    CSIMPLY(LOG_TICKET_ORDER_CREATE, fmt , ##args)
#define CTICKET_ORDER_PAY_LOG(fmt, args...)    CSIMPLY(LOG_TICKET_ORDER_PAY, fmt , ##args)
#define CTICKET_ORDER_TOKEN_LOG(fmt, args...)    CSIMPLY(LOG_TICKET_ORDER_TOKEN, fmt , ##args)

#define CASYNC_BUSINESS_LOG(fmt, args...)   CSIMPLY(LOG_ASYNC_BUSINESS, fmt , ##args)

#ifdef DEBUG //to make CDEBUG_LOG work, define DEBUG in Makefile
#define CDEBUG_LOG(fmt, args...)    CDETAIL(LOG_DEBUG, fmt , ##args)
#define CTRACE_LOG(fmt, args...)    CDETAIL(LOG_TRACE, fmt , ##args)
// #define CDEBUG_LOG(fmt, args...)    CSIMPLY(LOG_DEBUG, fmt , ##args)
// #define CTRACE_LOG(fmt, args...)    CSIMPLY(LOG_TRACE, fmt , ##args)
#else
//#define CDEBUG_LOG(fmt, args...)    ((void)0)
//#define CTRACE_LOG(fmt, args...)    ((void)0)

#define CDEBUG_LOG(fmt, args...)    \
    CDETAIL(LOG_DEBUG, fmt , ##args)
    // if(0){CSIMPLY(LOG_DEBUG, fmt , ##args);}

#define CTRACE_LOG(fmt, args...)    \
    CDETAIL(LOG_TRACE, fmt , ##args)
    // if(0){CSIMPLY(LOG_TRACE, fmt , ##args);}

#endif

#define CBOOT_LOG(OK, fmt , args...) do{ \
    c_boot_log(OK, 0, fmt , ##args); \
    return OK; \
}while (0)

#define CBOOT_LOG2(OK, n, fmt , args...) do{ \
    c_boot_log(OK, n, fmt , ##args); \
    return OK; \
}while (0)

#define CBOOT_LOG_NORETURN(OK, fmt , args...) do{ \
    c_boot_log(OK, 0, fmt , ##args); \
}while (0)



#define CERROR_RETURN(X, Y) do{ \
    CERROR_LOG X; \
    return Y; \
}while (0)

/*
 * log api
 */
__attr_cdecl__ __attr_nonnull__(1)
    extern int c_log_init (const char* dir, int level, u_int size, const char* fix_name);

__attr_cdecl__ __attr_nonnull__(2)
    extern void c_write_log (int lvl, const char* fmt, ...);

__attr_cdecl__ __attr_nonnull__(3)
    extern void c_boot_log (int OK, int dummy, const char* fmt, ...);

    extern void c_write_slicing(int lvl, const char* fun);

    __END_DECLS
#endif
