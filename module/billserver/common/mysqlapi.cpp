#include "mysqlapi.h"

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

/**
 * ִ�в�ѯ(����ȡ1����¼)
 * ����:  0:�޼�¼  1:�м�¼
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
        // ���ݲ�����
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
 * ִ�в�ѯ(��ȡ������¼)
 * ����:  0:�޼�¼  1:�м�¼
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
 * ִ�в��롢����
 * ����:  1:�ɹ�
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

