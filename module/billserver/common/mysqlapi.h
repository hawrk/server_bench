/**
  * FileName: mysqlapi.h 
  * Author: Created by jim 2017-05-08
  * History: 
  */
#ifndef _MYSQL_API_H_
#define _MYSQL_API_H_

#include <map>
#include <vector>
#include "log/clog.h"
#include "../../Base/Comm/clib_mysql.h"
#include "CExp.h"

using namespace std;


typedef map<string, string> SqlResultSet;
typedef vector<SqlResultSet> SqlResultMapVector;


/**
  * 本类对clib_mysql的接口进行了简单封装，主要包括如下功能:
  *     1、查询记录
  *     2、执行插入、更新语句
  */
class CMySQL
{
public:
    CMySQL();
    ~CMySQL();

	void Begin(clib_mysql& sql_instance) throw(CTrsExp);

	void Commit(clib_mysql& sql_instance) throw(CTrsExp);

	void Rollback(clib_mysql& sql_instance) throw(CTrsExp);

    /**
     * 执行查询(最多获取1条记录)
     * 返回:  0:无记录  1:有记录
     */
    int QryAndFetchResMap(clib_mysql& sql_instance, 
    						       const char * sql_str, 
    						       SqlResultSet & objMap) throw(CTrsExp);
	 /**
     * 执行查询(获取多条记录)
     * 返回:  0:无记录  1:有记录
     */
    int QryAndFetchResMVector(clib_mysql& sql_instance, 
    									 const char * sql_str, 
    									 SqlResultMapVector & objMapVector) throw(CTrsExp);
	 /**
     * 执行插入、更新
     * 返回:  1:成功
     */
	int Execute(clib_mysql& sql_instance, const char * sql_str) throw(CTrsExp);

	static char *ValiStr(char *str);

	int getAffectedRows(){ return m_iAffectedRows;}

	// 转义
	static string EscapeStr(const string & buff);

private:
	void Reset();
        
protected:
	int m_iAffectedRows;
};


#endif

