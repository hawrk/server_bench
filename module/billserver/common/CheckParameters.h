/******
    文件名: CheckParameters.h
    作者:
    最后修改日期:
    文件描述:
******/

#ifndef _CHECK_PARAMETERS_H_
#define _CHECK_PARAMETERS_H_

#include "log/clog.h"
#include "../business/apayErrorNo.h"
#include "../common/CCommFunc.h"
#include "speed_bill_protocol.h"

namespace Check
{

//校验金额   optional:选项，为true时必填
inline static void CheckMoney(const char * value_name, string & value,
                              long min = 0, long max = MAX_MONEY, bool optional=true)
{
    if(ISEMPTY(value))
    {
    	if(!optional)    //可选字段
        {
            return;
        }
		else
		{
			string strerrmsg = string("[") + string(value_name) + "] is empty!";
			CERROR_LOG("%s", strerrmsg.c_str());
			throw(CTrsExp(ERR_INVALID_PARAMS, strerrmsg));
		}
    }

    if(!ISDIGIT(value.c_str()) || !CHECK_LONG_RANAGE(STOL(value), min, max))
    {
		string strerrmsg = string("[") + value_name + "] is error!";
		CERROR_LOG("%s", strerrmsg.c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS, strerrmsg));
    }
}

//每页显示记录数limit   optional:选项，为true时必填
inline static void CheckLimit(string & value,bool optional=false) throw(CTrsExp)
{
    if(ISEMPTY(value))
    {
    	if(!optional)    //可选字段
        {
            return;
        }
		else
		{
			string strerrmsg = "[limit] is empty!";
			CERROR_LOG("%s", strerrmsg.c_str());
			throw(CTrsExp(ERR_INVALID_PARAMS, strerrmsg));
		}
    }

    if(!ISDIGIT(value.c_str()) || !CHECK_LONG_RANAGE(STOL(value), LIMIT_MIN, LIMIT_MAX))
    {
		string strerrmsg = "[limit] is error!";
		CERROR_LOG("%s", strerrmsg.c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS, strerrmsg));
    }
}

//当前页数page   optional:选项，为true时必填
inline static void CheckPage(string & value,bool optional=false) throw(CTrsExp)
{
    if(ISEMPTY(value))
    {
    	if(!optional)    //可选字段
        {
            return;
        }
		else
		{
			string strerrmsg = "[page] is empty!";
			CERROR_LOG("%s", strerrmsg.c_str());
			throw(CTrsExp(ERR_INVALID_PARAMS, strerrmsg));
		}
    }

    if(!ISDIGIT(value.c_str()) || !CHECK_LONG_RANAGE(STOL(value), PAGE_MIN, PAGE_MAX))
    {
		string strerrmsg = "[page] is error!";
		CERROR_LOG("%s", strerrmsg.c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS, strerrmsg));
    }
}

/*
 * add hawrk 2015.12.14
 * 检查字符串合法性，
 * 输入参数：tagname:字段名；tagvalue:字段值；min:最小长度; max:最大长度；optional：选项，为true时必填
 * 输出参数：无
 */
template <typename T>
inline static void CheckStrParam(const string& tagname,const string& tagvalue,const T& min,const T& max,bool optional=false)
{

    if(ISEMPTY(tagvalue))
    {
        if(!optional)    //选填
        {
            return;
        }
        else            //必填
        {
        	string strerrmsg = string("[") + tagname + "] is empty!";
			CERROR_LOG("%s", strerrmsg.c_str());
			throw(CTrsExp(ERR_INVALID_PARAMS, strerrmsg));
        }
    }

    if(!CHECK_STR_LENGTH(tagvalue.c_str(),min, max))
    {
    	string strerrmsg = string("[") + tagname + "] is error!";
		CERROR_LOG("%s", strerrmsg.c_str());
		throw(CTrsExp(ERR_INVALID_PARAMS, strerrmsg));
    }
}


/*
 * add hawrk 2015.12.14
 * 检查数字串合法性，检查是否为纯数字和检验范围
 * 输入参数：tagname:字段名；tagvalue:字段值；min:最小值; max:最大值；optional：选项，为true时必填
 *        min和max不填时不检验其取值范围
 * 输出参数：无
 */
inline static void CheckDigitalParam(const string& tagname, string& tagvalue,const int& min = 0,const int& max = 0,bool optional=false)
{
    CDEBUG_LOG("CheckDigitalParam :[%s]", tagvalue.c_str());
    if(ISEMPTY(tagvalue))
    {
        if(!optional)    //选填
        {
            return;
        }
        else            //必填
        {
        	string strerrmsg = string("[") + tagname + "] is empty!";
			CERROR_LOG("%s", strerrmsg.c_str());
            throw(CTrsExp(ERR_INVALID_PARAMS, strerrmsg));
        }
    }

    if(min == 0 && max == 0)
    {
        if( !ISDIGIT(tagvalue.c_str()))
        {
        	string strerrmsg = string("[") + tagname + "] is error!";
			CERROR_LOG("%s", strerrmsg.c_str());
            throw(CTrsExp(ERR_INVALID_PARAMS, strerrmsg));
        }
        return;
    }

    if( !ISDIGIT(tagvalue.c_str()) || !CHECK_INT_RANGE(STOI(tagvalue), min, max))
    {
    	string strerrmsg = string("[") + tagname + "] is error!";
		CERROR_LOG("%s", strerrmsg.c_str());
        throw(CTrsExp(ERR_INVALID_PARAMS, strerrmsg));
    }
}

/*
 * add hawrk 2015.12.14
 * 检查数字串合法性，检查是否为纯数字
 * 输入参数：tagname:字段名；tagvalue:字段值；optional：选项，为true时必填
 * 输出参数：无
 */
 template <typename T>
inline static void CheckIsDigitalParam(const string& tagname,const string& tagvalue,const T& min,const T& max,bool optional=false)
{
    if(ISEMPTY(tagvalue))
    {
        if(!optional)    //选填
        {
            return;
        }
        else            //必填
        {
        	string strerrmsg = string("[") + tagname + "] is empty!";
			CERROR_LOG("%s", strerrmsg.c_str());
            throw(CTrsExp(ERR_INVALID_PARAMS, strerrmsg));
        }
    }

    if( !ISDIGIT(tagvalue.c_str()) || !CHECK_STR_LENGTH(tagvalue.c_str(),min, max))
    {
    	string strerrmsg = string("[") + tagname + "] is error!";
		CERROR_LOG("%s", strerrmsg.c_str());
        throw(CTrsExp(ERR_INVALID_PARAMS, strerrmsg));
    }
}


}

#endif// _CHECK_PARAMETERS_H_
