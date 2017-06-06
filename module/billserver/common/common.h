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
 * 定义大整数类型
 */
typedef int64_t LONG;

/**
 * 查询结果赋值的宏定义
 */
#define ICPY(A, B) A = ((B) ? atoi(B) : 0);
#define LCPY(A, B) A = ((B) ? atoll(B) : 0);
#define SCPY(A, B) strncpy(A, ((B) == NULL) ? "" : (B), sizeof(A) - 1)

// SPID最大长度
const int MAX_SP_LEN = 10;

// 消息最大长度
//const int MAX_MSG_LEN = 1024 * 16;

// 消息最大长度
//const int MAX_RSP_MSG_LEN = 31500;

// SQL最大长度
//const int MAX_SQL_LEN = 1024 * 10;

// 参数值最大长度
//const int MAX_PARAM_LEN = 1024 * 16;

// 整数最大值
const int MAX_INTEGER = std::numeric_limits<int>::max();
const int MIN_INTEGER = std::numeric_limits<int>::min();

// 长整数最大值
const LONG MAX_LONG = std::numeric_limits<LONG>::max();
const LONG MIN_LONG = std::numeric_limits<LONG>::min();

/**
 * 判断是否是数字字符串
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
 * 将字符串转换为整数
 */
int toInt(const char* value);

/**
 * 将字符串转换为长整数
 */
LONG toLong(const char* value);

/**
 * 将字符串转换为小写
 */
string& toLower(string& str);

/**
 * 将字符串转换为小写
 */
char* toLower(char* sz);

/**
 * 将字符串转换为大写
 */
string& toUpper(string& str);

/**
 * 将字符串转换为大写
 */
char* toUpper(char* sz);

/**
 * 获取当前主机IP
 */
string getLocalHostIp();

/**
 * 将时间转换为系统时间
 */
time_t toUnixTime(const string& strTime);

time_t fundTimetoUnixTime(const string& strTime);

/**
 * @input YYYYMMDD
 * @output YYYY-MM-DD
 */
string toDate(const string& strDate);

/**
 * 获取系统时间
 */
string getSysTime();

/**
 * 获取系统时间
 */
string getSysTime(time_t t);

string getSysDate();

/**
 * 获取系统时间: YYYYMMDD
 */
string getSysDate(time_t t);

int getToday();

int GetYesterday();

int getStartTime(time_t tnow);

/**
 * 取时间的年部分
 */
int year(const string& str);

/**
 * 取时间的月部分
 */
int month(const string& str);

/**
 * 取时间的日部分
 */
int day(const string& str);

/**
 * 取当前日期
 *@output:   string   YYYYMM
 */
string nowdate(const string& str);

/**
 * 取当前日期
 *@output:   string   YYYYMM
 */
string lastmonth(const string& str);

/**
 * 取下一月日期
 *@output:   string   YYYYMM
 */
string nextdate(const string& str);

/**
 * 检查是否为数字
 */
int isNumString(const char *str);

/**
 * 指针转换
 */
const char *ValiStr(char *str);

/**
 * 指针转换
 */
const char *ValiDateStr(char *str);

/**
 *字符串折成整数数组
 */
void split(vector <int > & list, const char * sp);

/**
  * 字符串分割操作. 以splitter切割字符串
  * 字符串为空时返回空vector
  */
vector<string> split(const string &src, const char* splitter);

void split_ex(const string &src, const char ch, vector<string>& strv);


/**
 * 给字符串后缀一个下标
 */
string add_suffix(const char* name, int n);

/**
 * 将字符串去除空格
 */
string& strTrim(string& str);

/**
 * 将字符串去除空格特殊字符，只保留数字和字母
 */
string strTrimSpecial(const string& src);

/**
 * 计算开始时间和结束时间月份间隔
 */
int monthInterval(const string& s_time , const string& e_time) ;


/**
 * 字符串替换
 * @param rawStr 源字符串
 * @param from 被替的内容
 * @param to 替换的目的字符串
 * @return 替换后的字符串
 */
string str_replace(string rawStr,string from,string to);


/**
 * 去字串左边的空格
 * @param strValue 被去空格的字串
 */
void InplaceTrimLeft(std::string& strValue);

/**
 * 去字串右边的空格
 * @param strValue 被去空格的字串
 */
void InplaceTrimRight(std::string& strValue);

/**
 * 去字串左右两边的空格
 * @param strValue 被去空格的字串
 */
void InplaceTrim(std::string& strValue);

/**
 * 字符串分割
 * @param strMain 被用于分割的字符串
 * @param chSpliter 分割符
 * @param strList 分割后的多个字符串
 * @param bReserveNullString 是否保留空字符串，该参数为true时，分割的空字符串在strList中被保存返回
 */
void str_split(const std::string& strMain, char chSpliter,
    std::vector<std::string>& strList, bool bReserveNullString);


/*  hawrk -----废弃，使用tars_util/include/tc_file下面的copyfile
 * 复制文件
 * @param SourceFile 源文件
 * @param NewFile 目标文件
 */
//int CopyFile(const char *SourceFile,const char *NewFile);

int file_mmap(const std::string& map_file_name, vector<std::string>& vecBill);

#endif

