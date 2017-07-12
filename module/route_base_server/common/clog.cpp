#include <assert.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include "clog.h"
#include "mutex.h"

#define MAX_LOG_CNT    1000
static const int LOG_BUFFER_SIZE = 4096;
static int has_init = 0;
static int log_level;
static u_int log_size;
static char log_pre[32] = {0};
static char log_dir[256] = {0};

static struct fds_t
{
    int seq;
    int opfd;
    u_short day;
} fds_info[LOG_END + 1];

static char* log_buffer = (char*)(MAP_FAILED);
static pthread_mutex_t clog_mutex_lock = PTHREAD_MUTEX_INITIALIZER;

static inline void log_file_name(int lvl, int seq, char* file_name, size_t len, time_t now)
{
    struct tm tm;

    assert (lvl >= LOG_EMERG && lvl <= LOG_END);

    localtime_r(&now, &tm);
    snprintf (file_name, len, "%s/%s%s%04d%02d%02d%03d", log_dir, log_pre,
                    ((const char*[])
                    {
                        "emerg", "alert", "crit",
                        "error", "warn", "notice",
                        "info", "debug", "trace", "async_business",
                        "ticket_order_create", "ticket_order_pay", "ticket_token", "end"
                    })[lvl],
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, seq);
}

static int request_log_seq (int lvl)
{
    char file_name[FILENAME_MAX];
    int seq;
    time_t now = time (NULL);
    struct stat buf;
    /*
     * find last log and tag with seq
     */
    for (seq = 0; seq < MAX_LOG_CNT; seq ++) 
    {
        log_file_name (lvl, seq, file_name, sizeof(file_name), now);

        if (access (file_name, F_OK)) 
        {
            if (seq == 0)
            {
                return 0;
            }
            else
            {
                break;
            }
        }
    }

    if (seq == MAX_LOG_CNT) 
    {
        fprintf (stderr, "too many log files\n");
        return -1;
    }

    log_file_name (lvl, --seq, file_name, sizeof(file_name), now);
    if (stat (file_name, &buf)) 
    {
         fprintf (stderr, "stat %s error, %m\n", file_name);
         return -1;
    }

    return seq == 0 ? seq : seq - 1;
}

static int open_fd(int lvl, time_t now)
{
    char file_name[FILENAME_MAX];
    struct tm *tm;
    int val;

    log_file_name (lvl, fds_info[lvl].seq, file_name, sizeof(file_name), now);
    fds_info[lvl].opfd = open (file_name, O_WRONLY|O_CREAT|O_APPEND, 0644);
    if (fds_info[lvl].opfd > 0)
    {
        tm = localtime (&now);
        fds_info[lvl].day = tm->tm_yday;

        val = fcntl(fds_info[lvl].opfd, F_GETFD, 0);
        val |= FD_CLOEXEC;
        fcntl(fds_info[lvl].opfd, F_SETFD, val);
    }
    return fds_info[lvl].opfd;
}

static int shift_fd (int lvl, time_t now)
{
    off_t length;
    struct tm *tm;

    if (fds_info[lvl].opfd < 0 && unlikely (open_fd (lvl, now) < 0))
        return -1;

    length = lseek (fds_info[lvl].opfd, 0, SEEK_END);
    tm = localtime (&now);
    if (likely (length < log_size && fds_info[lvl].day == tm->tm_yday))
        return 0;

    close (fds_info[lvl].opfd);
    if ( fds_info[lvl].day != tm->tm_yday)
        fds_info[lvl].seq = 0;
    else
        fds_info[lvl].seq ++;

    return open_fd (lvl, now);
}

void c_boot_log (int OK, int dummy, const char* fmt, ...)
{
#define SCREEN_COLS    80
#define BOOT_OK        "\e[1m\e[32m[ ok ]\e[m"
#define BOOT_FAIL    "\e[1m\e[31m[ failed ]\e[m"
    int end, i, pos;
    va_list ap;

    if (log_buffer == MAP_FAILED) 
    {
        log_buffer = (char*)(mmap (0, LOG_BUFFER_SIZE, 
                                    PROT_WRITE | PROT_READ, 
                                    MAP_PRIVATE | MAP_ANONYMOUS, 
                                    -1, 0));
                                    
        if (log_buffer == MAP_FAILED)
        {
            fprintf (stderr, "mmap log buffer failed, %m\n");
            exit (-1);
        }
    }

    va_start(ap, fmt);
    end = vsnprintf (log_buffer, LOG_BUFFER_SIZE, fmt, ap);
    va_end (ap);

    pos = SCREEN_COLS - 10 - (end - dummy) % SCREEN_COLS;
    for (i = 0; i < pos; i++)
    {
        log_buffer[end + i] = ' ';
    }
    
    log_buffer[end + i] = '\0';

    strncat (log_buffer, OK == 0 ? BOOT_OK : BOOT_FAIL, LOG_BUFFER_SIZE-strlen(log_buffer)-1);
    printf ("\r%s\n", log_buffer);

    if (OK != 0)
        exit (OK);
}

void c_write_log (int lvl, const char* fmt, ...)
{
    struct tm tm;
    int pos, end;
    va_list ap;
    time_t now;
	
	Mutex cLocker(&clog_mutex_lock);

    if (unlikely (!has_init)) 
    {
        va_start(ap, fmt);
        switch (lvl)
        {
        case LOG_EMERG:
        case LOG_CRIT:
        case LOG_ALERT:
        case LOG_ERROR:
            vfprintf (stderr, fmt, ap);
            break;
        default:
            vfprintf (stdout, fmt, ap);
            break;
        }
        va_end(ap);
        return;
    }

    if (lvl > log_level)
        return ;

    now = time (NULL);
    if (unlikely (shift_fd (lvl, now) < 0))
        return ;

    localtime_r(&now, &tm);
//    if (lvl == LOG_INFO || lvl == LOG_NOTICE || lvl == LOG_WARNING)
//        pos = snprintf (log_buffer, LOG_BUFFER_SIZE, "[%02d:%02d:%02d]",
//            tm.tm_hour, tm.tm_min, tm.tm_sec);
//    else
    pos = snprintf (log_buffer, LOG_BUFFER_SIZE, "[%02d:%02d:%02d][%05d]",
        tm.tm_hour, tm.tm_min, tm.tm_sec, getpid());


    va_start(ap, fmt);
    end = vsnprintf (log_buffer + pos, LOG_BUFFER_SIZE - pos, fmt, ap);
    va_end(ap);
   
    //防止越界，写到日志文件导致乱码
    int writelen = end + pos;
	if( pos + end > LOG_BUFFER_SIZE - 1 )
	{
		log_buffer[LOG_BUFFER_SIZE - 2] = '\n';
        writelen = LOG_BUFFER_SIZE - 1;
	}
    
    write (fds_info[lvl].opfd, log_buffer, writelen);
    //write (fds_info[lvl].opfd, log_buffer, end + pos);
}

void c_write_slicing(int lvl, const char* func)
{
    int pos;
	
	Mutex cLocker(&clog_mutex_lock);

    if (unlikely (!has_init)) 
    {
        return;
    }

    pos = snprintf (log_buffer, LOG_BUFFER_SIZE,
                    "==========================================="
                    "BEGIN[%s]==========================================\n",
                    func);

    write (fds_info[lvl].opfd, log_buffer, pos);
}

int c_log_init (const char* dir, int lvl, u_int size, const char* pre_name)
{
    int i, ret_code = -1;

    if (lvl < 0 || lvl > LOG_END) {
        fprintf (stderr, "init log error, invalid log level=%d\n", lvl);
        goto exit_entry;
    }

    /*
     * log file is no larger than 2GB
     */
    if (size > 0x80000000U)
    {
        fprintf (stderr, "init log error, invalid log size=%d\n", size);
        goto exit_entry;
    }

    if (access (dir, W_OK))
    {
        fprintf (stderr, "access log dir %s error, %m\n", dir);
        goto exit_entry;
    }

    if (log_buffer == MAP_FAILED) 
    {
        log_buffer = (char*)(mmap (0, 4096, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        if (log_buffer == MAP_FAILED) {
            fprintf (stderr, "mmap log buffer failed, %m\n");
            goto exit_entry;
        }
    }

    strncpy (log_dir, dir, sizeof (log_dir) - 1);
    if (pre_name != NULL)
        strncpy (log_pre, pre_name, sizeof (log_pre) - 1);
    log_size = size;
    log_level = lvl;

    for (i = LOG_EMERG; i <= LOG_END; i++) 
    {
        fds_info[i].opfd = -1;
        fds_info[i].seq = request_log_seq (i);

        if (fds_info[i].seq  < 0)
            goto exit_entry;
    }

    has_init = 1;
    ret_code = 0;
exit_entry:
    CBOOT_LOG (ret_code, "set log dir %s, per file size %d MB", dir, size/1024/1024);
}


