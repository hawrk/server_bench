#ifndef _C_BILL_FILE_H_
#define _C_BILL_FILE_H_

#include <stdio.h>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>

using std::string;



/**
* 日志类
*/
class CBillFile
{
public:

	/**
	* 构造函数
	*/
	CBillFile(const char* path, time_t timeNow, int shift_mode = 0, const char* suffix = "");

	CBillFile(const char* path, const char* file_name);

	/**
	* 析构函数
	*/
	virtual ~CBillFile();

	int raw(const char *fmt, ...);
	/**
	* 打开文件
	*/
	int _open();

	int _access(std::string strFileName);
	/**
	* 关闭
	*/
	void _close();


	/**
	*
	*/
	int _write(const char *szLog, int len);

	/**
	* 确定文件名
	*/
	string _file_name();

	string GetFileName();

protected:


	/**
	* 文件指针
	*/
	int  _fd;

	/**
	* 文件路径
	*/
	string _path;


	/**
	* 进程ID
	*/
	int _proc_id;

	/**
	* 当前系统时间
	*/
	struct tm _tm_now;

	/**
	* 消息序列号
	*/
	string _msg_id;

	/**
	* 文件名后缀
	*/
	string _suffix;

	string fName;

	string m_sFileName;

	int _shift_mode;

	time_t tnow;

	/**
	* 获取错误信息
	*/
	char _szErrInfo[1024 + 1];
};

#endif


