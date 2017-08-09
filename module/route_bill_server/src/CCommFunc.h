#ifndef _COMM_FUNC_H_
#define _COMM_FUNC_H_

#include <sstream>
#include "../Base/Comm/url_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <algorithm>
#include "CCommFunc.h"
#include <openssl/md5.h>
#include "mysqlapi.h"
#include "../Base/Comm/openapi_client.h"
#include "openssl/rsa.h"
#include "openssl/pem.h"
#include "log/clog.h"

#define GET4BITRANDOM()                         Get4BitRandom()
#define LTOS(lNum)                  	        Long2String(lNum)
#define ITOS(iNum)                              Int2String(iNum)
#define STOI(szStr)                             String2Int(szStr)
#define STOL(szStr)                             String2Long(szStr)
#define STODOUBLE(szStr)                        String2Double(szStr)
#define ISDIGIT(szStr)                          IsDigitString(szStr)
#define CHECK_INT_RANGE(iPar,iMin,iMax)         CheckIntRange(iPar,iMin,iMax)
#define CHECK_INT_RANAGE(iPar,iMin,iMax)        CheckIntRanage(iPar,iMin,iMax)
#define CHECK_LONG_RANAGE(iPar,iMin,iMax)       CheckLongRanage(iPar,iMin,iMax)
#define CHECK_STR_LENGTH(szPar,iMinLen,iMaxLen) CheckStrLength(szPar,iMinLen,iMaxLen)
#define ISEMPTY(szStr)                          IsEmpty(szStr)
#define TIMECMP(sInTime)                        TimeCompare(sInTime)
#define TIMECMP2(sInTime1,sInTime2)             TimeCompare(sInTime1,sInTime2)

//获取当前系统时间格式为:YYYYmmddhhmmss
int GetSysTime(char *str);

//获取当前系统时间格式为:YYYY-mm-dd hh:mm:ss
void GetCurDateTime(string & str_cur_datetime);

string GetSysTimeUsecEx(time_t t);

//与系统时间比较,大于系统时间返回1，等于返回0，小于返回-1
int TimeCompare(string &sInTime);

//两时间比较,1: sInTime1 > sInTime2  0:sInTime1 = sInTime2  -1:sInTime1 < sInTime2
int TimeCompare(string &sInTime1, string &sInTime2);

//获取4位随机数
int Get4BitRandom();

// 数字转换成字符串
string Long2String(long lNum);

// 数字转换成字符串
string Int2String(int iNum);

// 字符串转换成数字
int String2Int(string str);

// 字符串转换成数字
long String2Long(string str);

// 字符串转换成double
double String2Double(string str);

template<class T>
string toString(const T &value)
{
    stringstream ss;
    ss<<value;
    return ss.str();
}


// 判断是否是数字字符串
bool IsDigitString(const char *szStr);

// 校验整数范围,必须连续
bool CheckIntRange(const int iPar, const int iMin, const int iMax);

// 校验整数范围,必须连续
bool CheckIntRanage(const int iPar, const int iMin, const int iMax);

// 校验整数范围,必须连续
bool CheckLongRanage(const long iPar, const long iMin, const long iMax);

//校验字符串长度范围
bool CheckStrLength(const char *szPar, const int iMinLen, const int iMaxLen);

//判断字符串是否为空
bool IsEmpty(const string &szString);

//map 转换成string
bool Map2Str(const map<string,string> &inMap, string &outStr);

//获取8位日期格式:YYYYMMDD
string GetDate();

//获取时间，返回格式:HHmmss
string GetTime();




string UnixTime2Date(string & szUnizTime);

string toDate(const string& strDate);

string GetYesterDate();

int Kv2Map(const string str, NameValueMap & iodat);		//把k-v串解析为map
int Map2Kv(NameValueMap & iodat, string & str,			//把map解析为k-v串
		   const string excp = string(""));			//excp格式:"|ex1|ex2|...|"

string getMd5(string str);

void MapFirstToLower( NameValueMap& mapInput,NameValueMap& mapOutput);

void MapVector2Sum(SqlResultMapVector& resMVector, SqlResultSet& resMap);

string SHA256RSASign(const string &content, const string &key);

bool SHA256RSAVerify(const string &content, const string &sign, const string &key);

string Y2F(const string szStr);						//yuan转fen
string F2Y(const string szStr);						//fen转yuan

/*二进制转换为16进制字符串(1个字节转换为用2个字符表示)
 * 输入:  pSrc:二进制buffer
 *        len:buffer的长度
 *        iCaseType:输出格式  1为大写,2为小写
 * 输出:  16进制字符串
 * */
string bin2hex(const unsigned char * const pSrc, int len, int iCaseType = 1);

/*16进制字符串转换为二进制
 * 输入:  d:目标缓冲
 *        s:16进制字符串缓冲
 *        len:16进制字符串缓冲的长度
 * */
unsigned char *hex2bin(unsigned char *d, const char *s, int len);

bool isFileExist(const string& file);

// 过滤用户输入的非法字符
string getSafeInput(string str);

void stringReplace(string &strsrc, const string &strfind, const string &strnew);

//将instr以sqr为分隔符拆分到outVec中
int SplitString(string &instr, vector<string> &outVec, char sqr);

#endif

