/*************************************************************************
    * File: DBPool.h
    * Brief: 
    * Author: ln
    * Mail: lin@steck.com
    * Created Time: Mon 16 Jan 2017 04:58:21 PM CST
 ************************************************************************/

#ifndef DBPOOL_H_
#define DBPOOL_H_

#include "clib_mysql.h"
#include <map>

class CDBPool {
public:
    CDBPool();
    virtual ~CDBPool();

    // 添加连接池对象
    int AddDBConn( const int& iDbNum, const int& iType,
                 const std::string& strIp, const int& iPort,
                 const std::string& strUser, const std::string& strPass );

    // 获取连接池对象
    clib_mysql* GetDBConn( const int& iDbNum, const int& iType );

    // 删除连接池对象
    int RelDBConn( const int& iDbNUm, const int& iType );

    // 获取连接池size
    int GetDbPoolSize( const int& iType );

	std::map<int, clib_mysql*> GetMasterDBPool()
	{
		return m_stMasDbMap;
	}

public:
    std::map<int, clib_mysql*> m_stMasDbMap;
    std::map<int, clib_mysql*> m_stSlaDbMap;
};

#endif /* DBPOOL_H_ */
