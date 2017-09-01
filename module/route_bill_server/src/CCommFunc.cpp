#include "CCommFunc.h"

//获取当前系统时间格式为:YYYYmmddhhmmss
int GetSysTime(char *str)
{
	time_t tt;
	struct tm stTm;

	tt = time(NULL);
	memset(&stTm, 0, sizeof(stTm));
	localtime_r(&tt, &stTm);
	sprintf(str, "%04d%02d%02d%02d%02d%02d", stTm.tm_year + 1900,
			stTm.tm_mon + 1, stTm.tm_mday, stTm.tm_hour, stTm.tm_min,
			stTm.tm_sec);

	return 0;
}

//获取当前系统时间格式为:YYYY-mm-dd hh:mm:ss
void GetCurDateTime(string & str_cur_datetime)
{
	time_t t = time(NULL);
	struct tm * pCurTm = localtime(&t);
	
	char tmpTime[30] = {0};
	sprintf(tmpTime, "%04d-%02d-%02d %02d:%02d:%02d", pCurTm->tm_year+1900, pCurTm->tm_mon+1, pCurTm->tm_mday,
					pCurTm->tm_hour, pCurTm->tm_min, pCurTm->tm_sec);

	str_cur_datetime = tmpTime;
}
 //取系统 当前时间，时间精确到毫秒后6位，用于产生银行方请求流水
string GetSysTimeUsecEx(time_t t)
{
    struct  tm tm_now;
    localtime_r(&t, &tm_now);
    char szTmp[256];

    struct timeval cTime;
    gettimeofday(&cTime, NULL);
    snprintf(szTmp, sizeof(szTmp), "%04d%02d%02d%02d%02d%02d%06ld",
        tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
        tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, cTime.tv_usec);

    return szTmp;
}

//与系统时间比较,大于系统时间返回1，等于返回0，小于返回-1
int TimeCompare(string &sInTime)
{
    char   szTime[50]={0};

    GetSysTime(szTime);
    
    string szSysTime(szTime);
    if(sInTime == szSysTime)
        return 0;
    else if(sInTime > szSysTime)
        return 1;
    else
        return -1;   
}

//两时间比较,1: sInTime1 > sInTime2  0:sInTime1 = sInTime2  -1:sInTime1 < sInTime2
int TimeCompare(string &sInTime1, string &sInTime2)
{
    if(sInTime1 == sInTime2)
        return 0;
    else if(sInTime1 > sInTime2)
        return 1;
    else
        return -1;
}

//获取4位随机数
int 
Get4BitRandom()
{
    int iRandom;
	srand((unsigned)time(NULL));  //add hawrk

    iRandom = random()%1000;
    return iRandom;
}

//将长整型转换成字符串
string
Long2String(long lNum)
{
    string result;
    stringstream sstream;

    sstream.str("");
    sstream << lNum;
    sstream >> result;
  
    return result;
}

//将整型转换成字符串
string
Int2String(int iNum)
{
    string result;
    stringstream sstream;

    sstream.str("");
    sstream << iNum;
    sstream >> result;
  
    return result;
}

//将字符串转换成整型
int
String2Int(string szStr)
{
    int iResult=0;
    stringstream sstream;
    
    if(0 == szStr.length())
    {
        return 0;
    }
    
    sstream.str("");
    sstream << szStr;
    sstream >> iResult;

    return iResult;
}

//将字符串转换成整型
long
String2Long(string szStr)
{
    long iResult=0;
    stringstream sstream;
    
    if(0 == szStr.length())
    {
        return 0;
    }
    
    sstream.str("");
    sstream << szStr;
    sstream >> iResult;

    return iResult;
}

// 字符串转换成double
double String2Double(string str)
{
    double iResult=0;
    stringstream sstream;
    
    if(0 == str.length())
    {
        return 0;
    }
    
    sstream.str("");
    sstream << str;
    sstream >> iResult;

    return iResult;
}


//判断是否是数字字符串
bool 
IsDigitString(const char *szStr)
{
    const char* p = szStr;

    // 略过前导空格
    while(isspace(*p))  p++;

    // 略过数字
    while(isdigit(*p))  p++;

    // 略过末尾空格
    while(isspace(*p))  p++;
    
    return !(*p);
}

// 校验整数范围,必须连续
bool
CheckIntRange(const int iPar, const int iMin, const int iMax)
{
	return (iPar >= iMin && iPar <= iMax) ? true : false;
}

// 校验整数范围,必须连续
bool
CheckIntRanage(const int iPar, const int iMin, const int iMax)
{
    return (iPar >= iMin && iPar <= iMax)? true : false;
}

// 校验整数范围,必须连续
bool
CheckLongRanage(const long iPar, const long iMin, const long iMax)
{
    return (iPar >= iMin && iPar <= iMax)? true : false;
}

// 校验字符串长度有效范围
bool 
CheckStrLength(const char *szPar, const int iMinLen, const int iMaxLen)

{
    if(szPar == NULL)
    {
        return false;
    }

    int iLen = strlen(szPar);
    return (iLen >= iMinLen && iLen <= iMaxLen)?true:false;
}

//判空操作
bool
IsEmpty(const string &szString)
{
    return (szString.length() == 0)?true:false;
}

// 把map<string, string>转换成"k1=v1&k2=v2&k3=v3"类型的string
bool
Map2Str(const map<string, string> & inMap, string & outStr)
{
    unsigned int iLoopCnt = 0;
    map<string, string>::size_type itemNum;
    bool bRet = false;
    outStr = "";

    itemNum = inMap.size();
    map<string, string>::const_iterator it;
    for ( it = inMap.begin();  it != inMap.end();  ++it )
    {
        ++iLoopCnt;
		if ( ("sign" == it->first) || ( 0== it->second.length() ) )
		{
			continue;
		}

        bRet = true;

		if ( 1 != iLoopCnt )
		{
			outStr += "&";
		}
        outStr += it->first;
        outStr += "=";
        outStr += it->second;
    }

    return bRet;
}

string 
GetDate()
{
	time_t tt;
	struct tm stTm;
    char stDate[50]={0};
    
	tt = time(NULL);
	memset(&stTm, 0, sizeof(stTm));
	localtime_r(&tt, &stTm);
	sprintf(stDate, "%04d%02d%02d", stTm.tm_year + 1900,stTm.tm_mon + 1, stTm.tm_mday);

    string sDate = string(stDate);
	return sDate;
}

string 
GetTime()
{
	time_t tt;
	struct tm stTm;
    char cTime[50]={0};
    
	tt = time(NULL);
	memset(&stTm, 0, sizeof(stTm));
	localtime_r(&tt, &stTm);
	sprintf(cTime, "%02d%02d%02d",stTm.tm_hour, stTm.tm_min, stTm.tm_sec);

    string szTime = string(cTime);
	return szTime;
}


string UnixTime2Date(string & szUnizTime)
{
	struct tm stTm;
    char stDate[50]={0};

	time_t tt;
	tt = STOI(szUnizTime);
	
	memset(&stTm, 0, sizeof(stTm));
	localtime_r(&tt, &stTm);
	sprintf(stDate, "%04d%02d%02d", stTm.tm_year + 1900,stTm.tm_mon + 1, stTm.tm_mday);

    string sDate = string(stDate);
	return sDate;
}

string toDate(const string& strDate)
{
    int year, month, day;
    sscanf(strDate.c_str(), "%04d%02d%02d", &year, &month, &day);

    char szTmp[11];
    snprintf(szTmp, sizeof(szTmp), "%04d-%02d-%02d", year, month, day);

    return szTmp;
}

string toDateEx(const string& strDate)
{
    int year, month, day;
    sscanf(strDate.c_str(), "%04d-%02d-%02d", &year, &month, &day);

    char szTmp[11];
    snprintf(szTmp, sizeof(szTmp), "%04d%02d%02d", year, month, day);

    return szTmp;
}


string GetYesterDate()
{
	time_t tt;
	struct tm stTm;
    char stDate[50]={0};

	tt = time(NULL);
	memset(&stTm, 0, sizeof(stTm));
	localtime_r(&tt, &stTm);
	sprintf(stDate, "%04d-%02d-%02d", stTm.tm_year + 1900,stTm.tm_mon +1, stTm.tm_mday-1);

    string sDate = string(stDate);
	return sDate;
}


int Kv2Map(const string str, NameValueMap & iodat)	//把k-v串解析为map
{
	int num = 0;
	string::size_type keyleft = 0, keyright = 0;
	string::size_type valleft = 0, valright = 0;
	string key, val;

	//利用4个标志位, 找到key和value的首末位置, 再存入map
	while(valright != string::npos)
	{
		keyleft = str.find_first_not_of('&', valright);
		keyright = str.find_first_of('=', keyleft);
		valleft = str.find_first_not_of('=', keyright);
		valright = valleft==string::npos?valleft:str.find_first_of('&', valleft);

		key = str.substr(keyleft, keyright - keyleft);
		val = (valright == valleft)? "" : str.substr(valleft, valright - valleft);

		iodat[key] = val;
		++num;
	}

	return num;
}

int Map2Kv(NameValueMap & iodat, string & str, const string excp)
{
	int num = 0;
	stringstream sstr("");

	for(NameValueMap::iterator iter = iodat.begin(); iter != iodat.end(); ++iter)
	{
		if(iter->second == ""
		|| excp.find(string("|")+iter->first+string("|")) != string::npos)
			continue;
		++num;
		sstr << "&" << iter->first << "=" << iter->second;
	}

	if(num)
		str = sstr.str().substr(1);

	return num;
}

string getMd5(string str)
{
    MD5_CTX ctx;  

    unsigned char md[16]; 
    char szRes[32 + 1] = { 0 };

    MD5_Init(&ctx);
    MD5_Update(&ctx,str.c_str(),str.size());
    MD5_Final(md,&ctx);  
  
    for(int i=0; i<16; i++ )
    {  
        snprintf(szRes + i * 2, 3, "%2.2X", md[i]); // 大写
    } 
    
    szRes[32] = '\0'; 
    string strSign(szRes);
    return strSign;
}


void MapFirstToLower( NameValueMap& mapInput,NameValueMap& mapOutput)
{
	NameValueMapIter iter;
	for(iter=mapInput.begin();iter!=mapInput.end();iter++)
	{
		string szName = iter->first;
		transform(szName.begin(), szName.end(), szName.begin(), ::tolower); 
		if(szName == "notify_url" ||  szName == "notify_data")
		{
			mapOutput[szName] = iter->second;
		}
		else
		{
			mapOutput[szName] = getSafeInput(iter->second);
		}		
	}
}

void MapVector2Sum(SqlResultMapVector& resMVector, SqlResultSet& resMap)
{
	if ( 0 != resMVector.size() )
	{
		for(SqlResultMapVector::iterator it = resMVector.begin(); it != resMVector.end(); it++)
		{
			SqlResultSet mapTmp = *it;
			for(SqlResultSet::iterator itMap = mapTmp.begin(); itMap != mapTmp.end(); itMap++)
			{
				resMap[itMap->first] = LTOS(STOL(resMap[itMap->first]) + STOL(itMap->second));
			}
		}
	}
}

string SHA256RSASign(const string &content, const string &key) {

    string signed_str;
    const char *key_cstr = key.c_str();
	BIO *p_key_bio = NULL;
	p_key_bio = BIO_new(BIO_s_file());
	BIO_read_filename(p_key_bio, key_cstr);
	RSA *p_rsa = PEM_read_bio_RSAPrivateKey(p_key_bio, NULL, NULL, NULL);
    if (p_rsa != NULL){
        const char *cstr = content.c_str();
        unsigned char hash[SHA256_DIGEST_LENGTH] = {0};
        SHA256((unsigned char *)cstr, strlen(cstr), hash);
			
        unsigned char sign[XRSA_KEY_BITS / 8] = {0};
        unsigned int sign_len = sizeof(sign);
        int r = RSA_sign(NID_sha256, hash, SHA256_DIGEST_LENGTH, sign, &sign_len, p_rsa);

        if (0 != r && sizeof(sign) == sign_len) {
            signed_str = bin2hex(sign, sign_len, 2);
        }
    }

    RSA_free(p_rsa);
    BIO_free(p_key_bio);
    return signed_str;
}

bool SHA256RSAVerify(const string &content, const string &sign, const string &key) {

    bool result = false;
    const char *key_cstr = key.c_str();
	BIO *p_key_bio = NULL;
	p_key_bio = BIO_new(BIO_s_file());
	BIO_read_filename(p_key_bio, key_cstr);

    RSA *p_rsa = PEM_read_bio_RSA_PUBKEY(p_key_bio, NULL, NULL, NULL);

    if (p_rsa != NULL) {
        const char *cstr = content.c_str();
		
        unsigned char hash[SHA256_DIGEST_LENGTH] = {0};
        SHA256((unsigned char *)cstr, strlen(cstr), hash);
		
        unsigned char sign_cstr[XRSA_KEY_BITS / 8] = {0};
		
        hex2bin(sign_cstr, sign.c_str(), sign.length());
        unsigned int sign_len = XRSA_KEY_BITS / 8;
        
        int r = RSA_verify(NID_sha256, hash, SHA256_DIGEST_LENGTH, (unsigned char *)sign_cstr, sign_len, p_rsa);

        if (r > 0) {
            result = true;
        }
    }

    RSA_free(p_rsa);
    BIO_free(p_key_bio);
    return result;
}

string Y2F(const string szStr)	//yuan转fen
{
	stringstream sstream("");
	double lYuan;
	long lFen;
	sstream << szStr;
	sstream >> lYuan;

	//added begin
	double d_fen = lYuan * 100;
	d_fen = (long)(d_fen*100 + 50)/100.0;
	lFen = d_fen;
	//added end

	//lFen = lYuan * 100;
	sstream.clear();
	sstream.str("");
	sstream << lFen;
	return sstream.str();
}

string F2Y(const string szStr)	//fen转yuan
{
	if( szStr.empty() )
	{
		return string("");
	}

	if( atol(szStr.c_str())<1 )
	{
		return string("");
	}

	char tmpBuf[50]={0};

	sprintf(tmpBuf,"%.2f",(atol(szStr.c_str())*1.0)/100);

	return string(tmpBuf); 
}

string bin2hex(const unsigned char * const pSrc, int len, int iCaseType)
{
    int iLen = len*2 + 1;
    char * pRes = new char[iLen];
    memset(pRes, 0x00, iLen);
    string strPrtFormt;
    if ( 1 == iCaseType )
    {   
            strPrtFormt = "%02X";
    }   
    else if ( 2 == iCaseType )
    {
            strPrtFormt = "%02x";
    }
    else
    {
            return string("");
    }

    for ( int i = 0; i < len; i++ )
    {

            sprintf(pRes + i*2, strPrtFormt.c_str(), pSrc[i]);
    }

    string strRes(pRes);
    if ( NULL != pRes )
    {
            delete [] pRes;
            pRes = NULL;
    }

    return strRes;
}

unsigned char *hex2bin(unsigned char *d, const char *s, int len) 
{ 
   int i, val; 
   if (len % 2 || !len) 
       return NULL; 
   memset (d, 0, len / 2); 
   for (i = 0; i < len; i++) { 
       if (s[i] >= '0' && s[i] <= '9') 
           val = s[i] - '0'; 
       else if (s[i] >= 'A' && s[i] <= 'F') 
           val = s[i] - 'A' + 10; 
       else if (s[i] >= 'a' && s[i] <= 'f') 
           val = s[i] - 'a' + 10; 
       else 
           return NULL; 

       d[i/2] |= val;
       if (! (i % 2))
           d[i/2] <<= 4;
   }
   return d;
}

bool isFileExist(const string& file)
{
	ifstream readfile;
	readfile.open(file.c_str(), ios::in);
	if( !readfile )
	{
		return false;
	}

	readfile.close();
	return true;
}

// 过滤用户输入的非法字符
string getSafeInput(string str)
{
	if ("" == str || str.length() <= 0)
	{
		return string("");
	}

	string tmpstr = str;

	stringReplace(tmpstr,"\r\n","");
	stringReplace(tmpstr,"&","＆");
	stringReplace(tmpstr,"|","｜");
	stringReplace(tmpstr,";","；");
	stringReplace(tmpstr,"$","");
	stringReplace(tmpstr,"%","");
	// stringReplace(tmpstr,"@","＃");
	stringReplace(tmpstr,"<","《");
	stringReplace(tmpstr,">","》");
	stringReplace(tmpstr,")","）");
	stringReplace(tmpstr,"(","（");
	stringReplace(tmpstr,"+","");
	stringReplace(tmpstr,"0x0d","");
	stringReplace(tmpstr,"0x0a","");
	stringReplace(tmpstr,",","，");
	stringReplace(tmpstr,"#","＃");
	stringReplace(tmpstr,"=","＝");
	stringReplace(tmpstr,"\"","”");
	stringReplace(tmpstr,"'","‘");
	stringReplace(tmpstr,"\\","＼");
	stringReplace(tmpstr,"0x4f","");
	stringReplace(tmpstr," ","");

	return tmpstr;
}

void stringReplace(string &strsrc, const string &strfind, const string &strnew)
{
	string::size_type pos=0;
	string::size_type srclen=strfind.size();
	string::size_type dstlen=strnew.size();
	while( (pos=strsrc.find(strfind, pos)) != string::npos)
	{
		strsrc.erase(pos, srclen);
		strsrc.insert(pos,strnew);
		pos += dstlen;
	}
}

//将instr以sqr为分隔符拆分到outVec中
int SplitString(string &instr, vector<string> &outVec, char sqr)
{
	string::iterator iterTmp, iterBegin;
	int cnt = 0;

	outVec.clear();
	for (iterBegin = iterTmp = instr.begin(); iterTmp != instr.end();)
	{
		if (*iterTmp == sqr)
		{
			outVec.push_back(string(iterBegin, iterTmp));
			iterBegin = ++iterTmp;
			cnt++;
			continue;
		}
		++iterTmp;
	}
	if (iterBegin != instr.end())
	{
		outVec.push_back(string(iterBegin, instr.end()));
		cnt++;
	}

	return (cnt);
}

