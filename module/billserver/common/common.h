#ifndef _COMMON_H_
#define _COMMON_H_

#include <string>
#include <vector>
#include "error.h"
#include <map>
#include <limits>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "limits.h"
#include <sstream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <iconv.h>

using std::stringstream;
using std::map;
using std::string;
using std::vector;

typedef map<string,string>CStr2Map;
typedef  vector<string> TstrVec;
typedef  map<string,TstrVec> CStr2sVec;

/**
 * �������������
 */
typedef int64_t LONG;

/**
 * ��ѯ�����ֵ�ĺ궨��
 */
#define ICPY(A, B) A = ((B) ? atoi(B) : 0);
#define LCPY(A, B) A = ((B) ? atoll(B) : 0);
#define SCPY(A, B) strncpy(A, ((B) == NULL) ? "" : (B), sizeof(A) - 1)

// SPID��󳤶�
const int MAX_SP_LEN = 10;

// ��Ϣ��󳤶�
//const int MAX_MSG_LEN = 1024 * 16;

// ��Ϣ��󳤶�
//const int MAX_RSP_MSG_LEN = 31500;

// SQL��󳤶�
//const int MAX_SQL_LEN = 1024 * 10;

// ����ֵ��󳤶�
//const int MAX_PARAM_LEN = 1024 * 16;

// �������ֵ
const int MAX_INTEGER = std::numeric_limits<int>::max();
const int MIN_INTEGER = std::numeric_limits<int>::min();

// ���������ֵ
const LONG MAX_LONG = std::numeric_limits<LONG>::max();
const LONG MIN_LONG = std::numeric_limits<LONG>::min();

/**
 * �ж��Ƿ��������ַ���
 */
bool isDigitString(const char *str);

template<class T>
string toString(const T &value)
{
    stringstream ss;
    ss<<value;
    return ss.str();
}


/**
 * ���ַ���ת��Ϊ����
 */
int toInt(const char* value);

/**
 * ���ַ���ת��Ϊ������
 */
LONG toLong(const char* value);

/**
 * ���ַ���ת��ΪСд
 */
string& toLower(string& str);

/**
 * ���ַ���ת��ΪСд
 */
char* toLower(char* sz);

/**
 * ���ַ���ת��Ϊ��д
 */
string& toUpper(string& str);

/**
 * ���ַ���ת��Ϊ��д
 */
char* toUpper(char* sz);

/**
 * ��ȡ��ǰ����IP
 */
string getLocalHostIp();

/**
 * ��ʱ��ת��Ϊϵͳʱ��
 */
time_t toUnixTime(const string& strTime);

time_t fundTimetoUnixTime(const string& strTime);

/**
 * @input YYYYMMDD
 * @output YYYY-MM-DD
 */
string toDate(const string& strDate);

/**
 * ��ȡϵͳʱ��
 */
string getSysTime();

/**
 * ��ȡϵͳʱ��
 */
string getSysTime(time_t t);

string getSysDate();

/**
 * ��ȡϵͳʱ��: YYYYMMDD
 */
string getSysDate(time_t t);

int getToday();

int GetYesterday();

int getStartTime(time_t tnow);

/**
 * ȡʱ����겿��
 */
int year(const string& str);

/**
 * ȡʱ����²���
 */
int month(const string& str);

/**
 * ȡʱ����ղ���
 */
int day(const string& str);

/**
 * ȡ��ǰ����
 *@output:   string   YYYYMM
 */
string nowdate(const string& str);

/**
 * ȡ��ǰ����
 *@output:   string   YYYYMM
 */
string lastmonth(const string& str);

/**
 * ȡ��һ������
 *@output:   string   YYYYMM
 */
string nextdate(const string& str);

/**
 * ����Ƿ�Ϊ����
 */
int isNumString(const char *str);

/**
 * ָ��ת��
 */
const char *ValiStr(char *str);

/**
 * ָ��ת��
 */
const char *ValiDateStr(char *str);

/**
 *�ַ����۳���������
 */
void split(vector <int > & list, const char * sp);

/**
  * �ַ����ָ����. ��splitter�и��ַ���
  * �ַ���Ϊ��ʱ���ؿ�vector
  */
vector<string> split(const string &src, const char* splitter);

void split_ex(const string &src, const char ch, vector<string>& strv);


/**
 * ���ַ�����׺һ���±�
 */
string add_suffix(const char* name, int n);

/**
 * ���ַ���ȥ���ո�
 */
string& strTrim(string& str);

/**
 * ���ַ���ȥ���ո������ַ���ֻ�������ֺ���ĸ
 */
string strTrimSpecial(const string& src);

/**
 * ���㿪ʼʱ��ͽ���ʱ���·ݼ��
 */
int monthInterval(const string& s_time , const string& e_time) ;


/**
 * �ַ����滻
 * @param rawStr Դ�ַ���
 * @param from ���������
 * @param to �滻��Ŀ���ַ���
 * @return �滻����ַ���
 */
string str_replace(string rawStr,string from,string to);


/**
 * ȥ�ִ���ߵĿո�
 * @param strValue ��ȥ�ո���ִ�
 */
void InplaceTrimLeft(std::string& strValue);

/**
 * ȥ�ִ��ұߵĿո�
 * @param strValue ��ȥ�ո���ִ�
 */
void InplaceTrimRight(std::string& strValue);

/**
 * ȥ�ִ��������ߵĿո�
 * @param strValue ��ȥ�ո���ִ�
 */
void InplaceTrim(std::string& strValue);

/**
 * �ַ����ָ�
 * @param strMain �����ڷָ���ַ���
 * @param chSpliter �ָ��
 * @param strList �ָ��Ķ���ַ���
 * @param bReserveNullString �Ƿ������ַ������ò���Ϊtrueʱ���ָ�Ŀ��ַ�����strList�б����淵��
 */
void str_split(const std::string& strMain, char chSpliter,
    std::vector<std::string>& strList, bool bReserveNullString);


/*  hawrk -----������ʹ��tars_util/include/tc_file�����copyfile
 * �����ļ�
 * @param SourceFile Դ�ļ�
 * @param NewFile Ŀ���ļ�
 */
//int CopyFile(const char *SourceFile,const char *NewFile);

int file_mmap(const std::string& map_file_name, vector<std::string>& vecBill);

#endif

