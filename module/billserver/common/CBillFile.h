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
* ��־��
*/
class CBillFile
{
public:

	/**
	* ���캯��
	*/
	CBillFile(const char* path, time_t timeNow, int shift_mode = 0, const char* suffix = "");

	CBillFile(const char* path, const char* file_name);

	/**
	* ��������
	*/
	virtual ~CBillFile();

	int raw(const char *fmt, ...);
	/**
	* ���ļ�
	*/
	int _open();

	int _access(std::string strFileName);
	/**
	* �ر�
	*/
	void _close();


	/**
	*
	*/
	int _write(const char *szLog, int len);

	/**
	* ȷ���ļ���
	*/
	string _file_name();

	string GetFileName();

protected:


	/**
	* �ļ�ָ��
	*/
	int  _fd;

	/**
	* �ļ�·��
	*/
	string _path;


	/**
	* ����ID
	*/
	int _proc_id;

	/**
	* ��ǰϵͳʱ��
	*/
	struct tm _tm_now;

	/**
	* ��Ϣ���к�
	*/
	string _msg_id;

	/**
	* �ļ�����׺
	*/
	string _suffix;

	string fName;

	string m_sFileName;

	int _shift_mode;

	time_t tnow;

	/**
	* ��ȡ������Ϣ
	*/
	char _szErrInfo[1024 + 1];
};

#endif


