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

#define RET_HASREC     1
#define RET_HASNOREC   0
#define RET_FAIL      -1

#define	NSTR	""


/**
  * �����clib_mysql�Ľӿڽ����˼򵥷�װ����Ҫ�������¹���:
  *     1����ѯ��¼
  *     2��ִ�в��롢�������
  */
class CMySQL
{
public:
    CMySQL();
    ~CMySQL();

    /**
     * ִ�в�ѯ(����ȡ1����¼)
     * ����:  0:�޼�¼  1:�м�¼
     */
    int QryAndFetchResMap(clib_mysql& sql_instance, 
    						       const char * sql_str, 
    						       SqlResultSet & objMap) throw(CTrsExp);
	 /**
     * ִ�в�ѯ(��ȡ������¼)
     * ����:  0:�޼�¼  1:�м�¼
     */
    int QryAndFetchResMVector(clib_mysql& sql_instance, 
    									 const char * sql_str, 
    									 SqlResultMapVector & objMapVector) throw(CTrsExp);
	 /**
     * ִ�в��롢����
     * ����:  1:�ɹ�
     */
	int Execute(clib_mysql& sql_instance, const char * sql_str) throw(CTrsExp);

	static char *ValiStr(char *str);

	int getAffectedRows(){ return m_iAffectedRows;}

private:
	void Reset();
        
protected:
	int m_iAffectedRows;
};


#endif

