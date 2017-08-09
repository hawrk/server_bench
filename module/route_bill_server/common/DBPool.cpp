/*************************************************************************
    * File: DBPool.cpp
    * Brief: 
    * Author: ln
    * Mail: lin@sctek.com
    * Created Time: Mon 16 Jan 2017 04:45:22 PM CST
 ************************************************************************/

#include "DBPool.h"
#include "log/clog.h"
#include "clib_mysql.h"

CDBPool::CDBPool()
{
    //TODO
}

CDBPool::~CDBPool()
{
    //TODO
}

/*
 * brief: 添加DB连接池对象
 *
 * */
int CDBPool::AddDBConn( const int& iDbNum, const int& iType,
                 const std::string& strIp, const int& iPort,
                 const std::string& strUser, const std::string& strPass )
{
    std::map<int, clib_mysql*>::iterator itMap;
    int iRet = 0;

    // 设置写连接池
    if ( 1 == iType )
    {
        if ( (itMap = m_stMasDbMap.find(iDbNum)) != m_stMasDbMap.end() )
        {
            CERROR_LOG( "DbNum:[%d] is matched in DbPool", iDbNum );
            return -1;
        }

        clib_mysql* pDBConn = new clib_mysql();
        iRet = pDBConn->init( strIp.c_str(), iPort,
                strUser.c_str(), strPass.c_str() );
        if (iRet) {
            delete pDBConn;
            return iRet;
        }
        printf("dbconn host:[%s],port[%d]",pDBConn->ms_host,pDBConn->mi_port);

        m_stMasDbMap.insert( std::make_pair( iDbNum, pDBConn ) );
        printf( "AddDBConn ok! DbNum:[%d] m_stMasDbMap.size:[%ld].\n", iDbNum, m_stMasDbMap.size() );
    }

    // 设置读连接池
    if ( 2 == iType )
    {
        if ( (itMap = m_stSlaDbMap.find(iDbNum)) != m_stSlaDbMap.end() )
        {
            CERROR_LOG( "DbNum:[%d] is matched in DbPool", iDbNum );
            return -1;
        }

        clib_mysql* pDBConn = new clib_mysql();
        iRet = pDBConn->init( strIp.c_str(), iPort,
                strUser.c_str(), strPass.c_str() );
        if (iRet) {
            delete pDBConn;
            return iRet;
        }

        m_stSlaDbMap.insert( std::make_pair( iDbNum, pDBConn ) );
    }

    CDEBUG_LOG( "Create DbPool ok! HostIp:[%s] Port:[%d] User:[%s]",
                strIp.c_str(), iPort, strUser.c_str() );

    return 0;
}

/*
 * breif: 获取连接池中对象
 * param: iDbNum:db编号, iType:读写类型
 * out: clib_mysql对象
 * */
clib_mysql* CDBPool::GetDBConn( const int& iDbNum, const int& iType )
{
    std::map<int, clib_mysql*>::iterator itMap;

    // 获取写对象
    if ( 1 == iType )
    {
        printf( "GetDBConn m_stMasDbMap.size:[%ld] DbNum:[%d].\n", m_stMasDbMap.size(), iDbNum );
        if ( (itMap = m_stMasDbMap.find(iDbNum)) == m_stMasDbMap.end() )
        {
            CERROR_LOG( "GetDBConn failed! DbNum:[%d] no obj match!",
                    iDbNum );
            printf( "GetDBConn failed! m_stMasDbMap.size:[%ld] DbNum:[%d].\n", m_stMasDbMap.size(), iDbNum );

            return NULL;
        }

    }

    // 获取读对象
    if ( 2 == iType )
    {
        if ( (itMap = m_stSlaDbMap.find(iDbNum)) == m_stSlaDbMap.end() )
        {
            CERROR_LOG( "GetDBConn failed! DbNum:[%d] no obj match!",
                    iDbNum );

            return NULL;
        }

    }

    return itMap->second;

}

/*
 * brief: 删除指定的连接池对象
 * param: iDbNum:指定数据库编号；iType:读写类型
 * */
int CDBPool::RelDBConn( const int& iDbNum, const int& iType )
{
    std::map<int, clib_mysql*>::iterator itMap;

    // 删除iDbNum对应写对象
    if ( 1 == iType )
    {
        if ( (itMap = m_stMasDbMap.find(iDbNum)) == m_stMasDbMap.end() )
        {
            CERROR_LOG( "RelDBConn failed! DbNum:[%d] no obj match!",
                    iDbNum );

            return -1;
        }

    }

    // 删除iDbNum对应读对象
    if ( 2 == iType )
    {
        if ( (itMap = m_stSlaDbMap.find(iDbNum)) == m_stSlaDbMap.end() )
        {
            CERROR_LOG( "RelDBConn failed! DbNum:[%d] no obj match!",
                    iDbNum );

            return -1;
        }

    }

    m_stSlaDbMap.erase( itMap );

    return 0;

}

/*
 * brief: 获取db连接池size
 * return: db连接池size
*/
int CDBPool::GetDbPoolSize( const int& iType )
{
    if ( 1 == iType )
    {
        return m_stMasDbMap.size();
    }
    else if ( 2 == iType )
    {
        return m_stSlaDbMap.size();
    }
    else
    {
        return -1;
    }
}

