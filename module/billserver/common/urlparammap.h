#ifndef _URL_PARAM_MAP_
#define _URL_PARAM_MAP_

#include <map>
#include <string>
#include <ostream>
#include <string.h>

using std::map;
using std::string;

/**
 * 声明(反)转义函数指针
 */
typedef string (*PFUN_ESCAPE)(const string&);
typedef string (*PFUN_UNESCAPE)(const string&);

/**
 * 缺省分隔、转义字符
 */
extern const char DFL_SEG_DEL;   
extern const char DFL_ELE_DEL;

/**
* 必须放在第一位的信息
*/
extern const char* __MUST_IN_FIRST_FIELD;

/**
 * 参数(Name-value)映射表
 */
class UrlParamMap: public std::map<string,string>
{
public:
    static char __SEG_DEL; // 段分隔符
    static char __ELE_DEL; // 名称-值分隔符
    static PFUN_ESCAPE __PFUN_ESCAPE; // 转义函数指针 
    static PFUN_UNESCAPE __PFUN_UNESCAPE; // 转义函数指针 
    static string encodeUrl(const string& str); // 缺省url转义函数
    static string decodeUrl(const string& str); // 缺省url反转义函数

	static int clib_urldecode_comm(const char *as_src, char *as_des, const int ai_len);
    
public:

    /**
     * 构造函数
     */
    UrlParamMap(const string& str="");

    /**
     * 解析URL参数串
     */
    void parseUrl(const string& strUrl);

	//void parseUrl(const string& strUrl, std::map<std::string, std::string>& strMap);
    
    /**
     * 拼装URL
     */
    void packUrl(string& strUrl);

protected:
    
    /**
     * 解析一个Name-value对
     */
    void parseNV(const string& strUrl, string::size_type beg, string::size_type end);
};

#endif

