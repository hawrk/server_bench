#include "CRouteBillBase.h"

string GetMchKey(const string& mch_id) throw(CTrsExp)
{
//	int iRet = 0;
//	ostringstream sqlss;
//	SqlResultSet mchInfo;
//
//	sqlss.str("");
//	sqlss << "select mch_id,bm_id,organ_id,mch_key,shop_name from "
//		<< SHOP_DB << "." << SHOPS_TABLE << " where "
//		<< "mch_id = '" << mch_id << "'"
//		<< ";";
//	clib_mysql * pShopDB = Singleton<CSpeedPosConfig>::GetInstance()->GetShopDB();
//	CMySQL mysql;
//	iRet = mysql.QryAndFetchResMap(*pShopDB, sqlss.str().c_str(), mchInfo);
//	if ( 0 == iRet )
//	{
//		CERROR_LOG("mch_id[%s] not exists!", mch_id.c_str());
//        throw(CTrsExp(ERR_MCH_NOT_EXISTS, "mch_id not exists!"));
//	}
//
//	return mchInfo["mch_key"];
	return "";
}

string GetMchSign(const string& mch_id,const string& szSrc) throw(CTrsExp)
{
	string szKey = GetMchKey(mch_id);
	string strSrcStr = ""; // 签名源串

	strSrcStr += szSrc + "&key=" + szKey;
	CDEBUG_LOG("strSrcStr=[%s]", strSrcStr.c_str());
	string newSign = getMd5(strSrcStr);	

	return newSign;
}


void CallIdServer(NameValueMap& reqMap,NameValueMap& resMap)throw(CTrsExp)
{
	CSocket* idSocket = Singleton<CSpeedPosConfig>::GetInstance()->GetIdServerSocket();

	NameValueMap resultMap;
    
    //char buf[10240] = {0};
	//idSocket->SendAndRecv(reqMap,buf,10240);
   
    //string szResString = string(buf);
    //Kv2Map(szResString,resultMap);

    idSocket->SendAndRecvLine(reqMap,resultMap,"\r\n");

	MapFirstToLower(resultMap,resMap);
}


void QryTableLoop(CDBPool* pDbPool,CMySQL &sqlHandle,string & szDbName,string &szTable,string &szSqlBuf1,string &szSqlBuf2,SqlResultMapVector &resultMVecter)
{
	//切换DB机器10张表，循环查询
	//CDBPool* pDbPool = Singleton<CSpeedPosConfig>::GetInstance()->GetDBPool();
	std::map<int, clib_mysql*> DbConMap = pDbPool->GetMasterDBPool();
	for (std::map<int, clib_mysql*>::iterator iter = DbConMap.begin(); iter != DbConMap.end(); ++iter)
	{
		clib_mysql* pDbCon = iter->second;
		for (int iDbIndex = 0; iDbIndex < 10; ++iDbIndex)
		{
			SqlResultMapVector tempMVecter;
			CDEBUG_LOG("pDbCon[%p] host[%s]! iDbIndex[%d]. \n", pDbCon, pDbCon->ms_host, iDbIndex);

			stringstream szSqlString("");
			szSqlString	<<	szSqlBuf1	<<	szDbName	<<	"."	<<	szTable	<<	"_"	<<	iDbIndex	<<	szSqlBuf2;
			
			//CMySQL cSqlHandle;
			sqlHandle.QryAndFetchResMVector(*pDbCon,szSqlString.str().c_str(),tempMVecter);

			vector<SqlResultSet>::iterator vIter = tempMVecter.begin();

			for(;vIter != tempMVecter.end();vIter++)
			{
				resultMVecter.push_back(*vIter);
			}			
		}
	
	}
}


void ParseOrderNo(string & szOrderNo,NameValueMap & resMap)
{
	//如果取到空值，抛错
	if(szOrderNo.empty())
	{
		CERROR_LOG("order_no in smpty!\n");
		throw(CTrsExp(SYSTEM_ERR,"Get order_no error!"));
	}

	resMap["db_name"]    = szOrderNo.substr(3,6);
	resMap["table_name"] = szOrderNo.substr(szOrderNo.length()-3,1);
	resMap["dbpool_no"]  = szOrderNo.substr(szOrderNo.length()-2,2);
}

void ParseJson2Map(const string& jsonStr, NameValueMap& strMap)
{
    //CDEBUG_LOG("Begin ...");

    CDEBUG_LOG("jsonStr=[%s]", jsonStr.c_str());
    JsonType jsonObj = JsonUtil::stringToObject(jsonStr);
    JsonMap jMap = jsonObj.toMap();
    //CDEBUG_LOG("jMap.size=[%d]", jMap.size());

    for( JsonMap::iterator it = jMap.begin(); it != jMap.end(); it++ )
    {
        if( JsonType::StringType == it->second.type() )
        {
            strMap[it->first.toString()] = it->second.toString();
        }
        else
        {
            strMap[it->first.toString()] = JsonUtil::objectToString(it->second);
        }
    }

    //CDEBUG_LOG("End.");
}


string GetJsonValue(const string& key, const JsonMap& ctJsonMap)
{
    JsonMap::const_iterator iterJson;

    if ((iterJson = ctJsonMap.find(key)) != ctJsonMap.end()){

        if (iterJson->second.type() == JsonType::StringType){
            if (iterJson->second.toString().empty()){
                return "";
            }
            return iterJson->second.toString();
        }
        else if (iterJson->second.type() == JsonType::NumberType)
        {
            return toString(iterJson->second.toNumber());
        }
    }

	return "";

}

int  SetOneFieldToXml(tinyxml2::XMLDocument * pDoc, tinyxml2::XMLNode* pXmlNode, const char * pcFieldName,
    const char* pszValue, bool bIsCdata)
{
    if (!pszValue || strlen(pszValue) == 0)
    {
        return 0;
    }

    if (!pDoc || !pXmlNode || !pcFieldName)
    {
        return -1;
    }

    tinyxml2::XMLElement * pFiledElement = pDoc->NewElement(pcFieldName);
    if (NULL == pFiledElement)
    {
        return -1;
    }

    tinyxml2::XMLText * pText = pDoc->NewText(pszValue);
    if (NULL == pText)
    {
        return -1;
    }

    pText->SetCData(bIsCdata);
    pFiledElement->LinkEndChild(pText);

    pXmlNode->LinkEndChild(pFiledElement);
    return 0;
}
