#include <stdarg.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <time.h>
#include <errno.h>
#include "CBillFile.h"

/**
* 每次记录的日志缓存
*/
#define WRITE_BILL(buf, str)\
	int iBufLen = 0; \
	const int iSize = sizeof(buf)-2; \
	time_t tnow = time(NULL); \
	localtime_r(&tnow, &_tm_now); \
	iBufLen = snprintf(buf, iSize, \
	"[%04d-%02d-%02d %02d:%02d:%02d][%d][%s][%s]", \
	_tm_now.tm_year + 1900, _tm_now.tm_mon + 1, \
	_tm_now.tm_mday, _tm_now.tm_hour, _tm_now.tm_min, _tm_now.tm_sec, \
	_proc_id, str, _msg_id.c_str()); \
	\
	va_list ap; \
	va_start(ap, fmt); \
	int iLength = vsnprintf(buf + iBufLen, iSize - iBufLen, fmt, ap); \
if (iLength >= iSize - iBufLen){ iBufLen = iSize; }\
	else if (iLength > 0){ iBufLen += iLength; }\
	va_end(ap); \
	buf[iBufLen++] = '\n'; \
	buf[iBufLen] = '\0'; \
	return _write(buf, iBufLen);

/**
* 构造函数
*/
CBillFile::CBillFile(const char* path, time_t timeNow, int shift_mode /* = 0 */, const char* suffix /* = "" */)
{
	// 成员初始化
	tnow = timeNow;
	_path = path;
	_suffix = suffix;
	_shift_mode = shift_mode;
	_proc_id = getpid();
	fName = "";
	_fd = -1;
}

CBillFile::CBillFile(const char* path, const char* file_name)
{
	// 成员初始化
	_path = path;
	fName = file_name;
	m_sFileName = _path + fName;
	_proc_id = getpid();
	_fd = -1;
}


/**
* 析构函数
*/
CBillFile::~CBillFile()
{
	// 关闭文件
	_close();
}

int CBillFile::raw(const char *fmt, ...)
{
	char buf[1024 * 4];
	int iBufLen = 0;

	va_list ap;
	va_start(ap, fmt);
	iBufLen += vsnprintf(buf + iBufLen, sizeof(buf)-iBufLen, fmt, ap);
	va_end(ap);

	return _write(buf, iBufLen);
}

/**
* 打开文件
*/
int CBillFile::_open()
{
	
	if (fName.empty())
	{
		m_sFileName = _file_name();
	}
	// 先关闭文件
	_close();

	// 打开文件
	if ((_fd = open(m_sFileName.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644)) < 0)
	{
		snprintf(_szErrInfo, sizeof(_szErrInfo), "open %s error:%s", m_sFileName.c_str(), strerror(errno));
		fprintf(stderr, "%s", _szErrInfo);
		_fd = -1;
		return -1;
	}

	return 0;
}

string CBillFile::GetFileName()
{
	return m_sFileName;
}

/**
* 关闭文件
*/
void CBillFile::_close()
{
	if (_fd != -1)
	{
		close(_fd);
		_fd = -1;
	}
}

int CBillFile::_access(std::string strFileName)
{
	if (access(m_sFileName.c_str(), F_OK) == 0)
	{
		return 1;
	}
	return 0;
}

/**
* 记录日志
*/
int CBillFile::_write(const char *str, int len)
{
	// 日志次数计数器
	// static int __count = 0;

	if (_fd == -1)
	{
		if (_open() != 0)
		{
			snprintf(_szErrInfo, sizeof(_szErrInfo), "open %s.log error:%s", _path.c_str(), strerror(errno));
			return -1;
		}
	}

	int ret = write(_fd, str, len);
	if (ret < 0)
	{
		_close();
		snprintf(_szErrInfo, sizeof(_szErrInfo), "puts %s.log error:%s", _path.c_str(), strerror(errno));
		return ret;
	}

	/*// 是否要执行日志切换
	if((__count++) >= SHIFT_FREQ)
	{
	_shift();
	__count = 0;
	}*/

	return 0;
}



/**
* 确定日志文件名
* @input:  index  文件索引编号
*/
string CBillFile::_file_name()
{
	char szSuffix[128] = { 0 };
	char szFile[256] = { 0 };
	//time_t tnow = time(NULL) - 24*3600;
	localtime_r(&tnow, &_tm_now);
	// 文件名后缀
	snprintf(szSuffix, sizeof(szSuffix), "_%s.csv", _suffix.c_str());

	if (_shift_mode == 2)
	{
		snprintf(szFile, sizeof(szFile), "%s_%04d%02d%02d%s", _path.c_str(), _tm_now.tm_year + 1900, _tm_now.tm_mon + 1, _tm_now.tm_mday, szSuffix);
	}
	else
	{
		snprintf(szFile, sizeof(szFile), "%s_%04d%02d%02d.csv", _path.c_str(), _tm_now.tm_year + 1900, _tm_now.tm_mon + 1, _tm_now.tm_mday);
	}

	return szFile;
}

