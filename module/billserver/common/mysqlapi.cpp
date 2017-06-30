#include "mysqlapi.h"

#define RET_HASREC     1
#define RET_HASNOREC   0
#define RET_FAIL      -1

#define	NSTR	""


CMySQL::CMySQL()
{
	m_iAffectedRows = 0;
}

CMySQL::~CMySQL()
{
}

void CMySQL::Reset()
{
	m_iAffectedRows = 0;
}

void CMySQL::Begin(clib_mysql& sql_instance) throw(CTrsExp)
{
	int iRet = 0;
	iRet = sql_instance.query("begin");
    if (iRet != 0)
    {
		CERROR_LOG("Transaction Begin Failed! Ret[%d] Err[%u~%s]",
            		iRet, sql_instance.get_errno(), sql_instance.get_error());
		throw(CTrsExp(sql_instance.get_errno(), sql_instance.get_error()));
    }
	CDEBUG_LOG("Transaction Begin ...");
}

void CMySQL::Commit(clib_mysql& sql_instance) throw(CTrsExp)
{
	int iRet = 0;
	iRet = sql_instance.query("commit");
    if (iRet != 0)
    {
		CERROR_LOG("Transaction Commit Failed! Ret[%d] Err[%u~%s]",
            		iRet, sql_instance.get_errno(), sql_instance.get_error());
		throw(CTrsExp(sql_instance.get_errno(), sql_instance.get_error()));
    }
	CDEBUG_LOG("Transaction Commit success.");
}

void CMySQL::Rollback(clib_mysql& sql_instance) throw(CTrsExp)
{
	int iRet = 0;
	iRet = sql_instance.query("rollback");
    if (iRet != 0)
    {
		CERROR_LOG("Transaction Rollback Failed! Ret[%d] Err[%u~%s]",
            		iRet, sql_instance.get_errno(), sql_instance.get_error());
		throw(CTrsExp(sql_instance.get_errno(), sql_instance.get_error()));
    }
	CDEBUG_LOG("Transaction Rollback success.");
}

/**
 * 执行查询(最多获取1条记录)
 * 返回:  0:无记录  1:有记录
 */
int CMySQL::QryAndFetchResMap(clib_mysql& sql_instance, 
						       const char * sql_str, 
						       SqlResultSet & objMap)
	throw(CTrsExp)
{
	objMap.clear();
	Reset();
	
	MYSQL_ROW row;
	MYSQL_FIELD * pField;
	int iNumCols;
	
	int iRet = 0;
	CDEBUG_LOG("Query sql = [%s]", sql_str);

	iRet = sql_instance.query(sql_str);
    if (iRet != 0)
    {
		CERROR_LOG("Query Failed! Ret[%d] Err[%u~%s]",
            		iRet, sql_instance.get_errno(), sql_instance.get_error());
		throw(CTrsExp(sql_instance.get_errno(), sql_instance.get_error()));
    }
	CDEBUG_LOG("Query ok! num_rows:[%d].", sql_instance.num_rows());

    if (sql_instance.num_rows() <= 0)
    {
        // 数据不存在
        return RET_HASNOREC;
    }

	try
	{
		if ((row = sql_instance.fetch_row()))
		{
			pField = sql_instance.fetch_fields();
			iNumCols = sql_instance.num_columns();

			for(int i = 0; i < iNumCols; i++)
			{
				CDEBUG_LOG("i=[%d], field=[%s], value=[%s]", i, pField[i].name, ValiStr(row[i]));
				objMap[pField[i].name] = ValiStr(row[i]);
			}

			sql_instance.free_result();

			return RET_HASREC;
		}
		else
		{		
			CERROR_LOG("fetch Failed! Ret[%d] Err[%u~%s]",
					iRet, sql_instance.get_errno(), sql_instance.get_error());
			throw(CTrsExp(sql_instance.get_errno(), sql_instance.get_error()));
		}
	}
	catch(CTrsExp e)
	{
		sql_instance.free_result();
		throw(CTrsExp(e.retcode, e.retmsg));
	}
}

/**
 * 执行查询(获取多条记录)
 * 返回:  0:无记录  1:有记录
 */
int CMySQL::QryAndFetchResMVector(clib_mysql& sql_instance, 
									 const char * sql_str, 
									 SqlResultMapVector & objMapVector)
	throw(CTrsExp)
{
	objMapVector.clear();
	Reset();

	MYSQL_ROW row;
	MYSQL_FIELD * pField;
	int iNumCols;
	int iNumRows;

	int iRet = 0;
	CDEBUG_LOG("Query sql = [%s]", sql_str);

	iRet = sql_instance.query(sql_str);
    if (iRet != 0)
    {
		CERROR_LOG("Query Failed! Ret[%d] Err[%u~%s]",
            		iRet, sql_instance.get_errno(), sql_instance.get_error());
        throw(CTrsExp(sql_instance.get_errno(), sql_instance.get_error()));
    }

	iNumRows = sql_instance.num_rows();
	CDEBUG_LOG("Query ok! num_rows:[%d].", sql_instance.num_rows());
	if (0 == iNumRows)
    {
        sql_instance.free_result();
		return RET_HASNOREC;
    }

	try
	{
		while((row = sql_instance.fetch_row()))
	    {
	        SqlResultSet mResultSet;
	        iNumCols = 0;
	        pField = sql_instance.fetch_fields();
	        iNumCols = sql_instance.num_columns();
	    
	        mResultSet.clear();
	        for(int i = 0; i < iNumCols; i++)
	        {
				//CDEBUG_LOG("i=[%d], field=[%s], value=[%s]", i, pField[i].name, ValiStr(row[i]));
	            mResultSet[pField[i].name] = ValiStr(row[i]);
	        }
	        objMapVector.push_back(mResultSet);
	    }
	}
	catch(...)
	{
		sql_instance.free_result();
		throw(CTrsExp(sql_instance.get_errno(), sql_instance.get_error()));
	}
	
	sql_instance.free_result();
	return RET_HASREC;
}

/**
 * 执行插入、更新
 * 返回:  1:成功
 */
int CMySQL::Execute(clib_mysql& sql_instance, const char * sql_str)
	throw(CTrsExp)
{
	Reset();
	int iRet = 0;
	CDEBUG_LOG("Execute sql = [%s]", sql_str);

	iRet = sql_instance.query(sql_str);
	if (iRet != 0)
    {
		CERROR_LOG("Execute Failed! Ret[%d] Err[%u~%s]",
            		iRet, sql_instance.get_errno(), sql_instance.get_error());
        throw(CTrsExp(sql_instance.get_errno(), sql_instance.get_error()));
    }

	m_iAffectedRows = sql_instance.affected_rows();
	return 1;
}

char * CMySQL::ValiStr(char *str)
{
	if (str == NULL)
		return (char *)NSTR;
	else
		return str;
}

// 转义
string
CMySQL::EscapeStr(const string & buff)
{
    int len = buff.length();
    char szTmp[len*2 + 1];
    memset(szTmp, 0, sizeof(szTmp));
    mysql_escape_string(szTmp, buff.c_str(), buff.length());
    
    return string(szTmp);
}

