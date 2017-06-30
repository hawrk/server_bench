#include "common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

using std::max;

/**
 * �ж��Ƿ��������ַ���
 */
bool isDigitString(const char *str)
{
    const char* p = str;

    // �Թ�ǰ���ո�
    while(isspace(*p))  p++;

    // �Թ�����
    while(isdigit(*p))  p++;

    // �Թ�ĩβ�ո�
    while(isspace(*p))  p++;

    return !(*p);
}

/**
 * ���ַ���ת��Ϊ����
 */
int toInt(const char* value)
{
    return value ? atoi(value) : 0;
}

/**
 * ���ַ���ת��Ϊ������
 */
LONG toLong(const char* value)
{
    return value ? atoll(value) : 0;
}

/**
 * ���ַ���ת��ΪСд
 */
string& toLower(string& str)
{
    for(string::iterator it=str.begin(); it != str.end(); ++it)
    {
        *it = tolower(*it);
    }

    return str;
}

/**
 * ���ַ���ת��ΪСд
 */
char* toLower(char* sz)
{
    for(char* p=sz; *p; p++)
    {
        *p = tolower(*p);
    }
    return sz;
}

/**
 * ���ַ���ת��Ϊ��д
 */
string& toUpper(string& str)
{
    for(string::iterator it=str.begin(); it != str.end(); ++it)
    {
        *it = toupper(*it);
    }

    return str;
}

/**
 * ���ַ���ת��Ϊ��д
 */
char* toUpper(char* sz)
{
    for(char* p=sz; *p; p++)
    {
        *p = toupper(*p);
    }
    return sz;
}

/**
 * ��ȡ��ǰ����IP
 */
string getLocalHostIp()
{
    int fd, intrface;
    long ip = -1;
    char szBuf[128] = {0};
    struct ifreq buf[16];
    struct ifconf ifc;

    if((fd=socket (AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        ifc.ifc_len = sizeof buf;
        ifc.ifc_buf = (caddr_t) buf;
        if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
        {
            intrface = ifc.ifc_len / sizeof(struct ifreq);
            while(intrface-- > 0)
            {
                if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface])))
                {
                    ip=inet_addr(inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));
                    break;
                }
            }
        }
        close (fd);
    }

    // ת��Ϊ.��ʽ
    unsigned char* pIp = (unsigned char*)(&ip);
    snprintf(szBuf, sizeof(szBuf), "%u.%u.%u.%u", pIp[0], pIp[1], pIp[2], pIp[3]);

    return szBuf;
}

/**
 * ��ʱ��ת��Ϊϵͳʱ��
 * @input       strTime     YYYY-MM-DD HH:MM:SS
 */
time_t toUnixTime(const string& strTime)
{
    // ȡ�ꡢ�¡��ն�
    int year=0, month=0, day=0, hour=0, minute=0, second=0;
    sscanf(strTime.c_str(), "%04d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second);

    // ������С��1900������0
    if(year < 1900)     return 0;

    // ת��Ϊ����ʱ��
    struct  tm tm_date;
    memset(&tm_date, 0, sizeof(tm));

    tm_date.tm_year =  year - 1900;
    tm_date.tm_mon = month - 1;
    tm_date.tm_mday = day;
    tm_date.tm_hour = hour;
    tm_date.tm_min = minute;
    tm_date.tm_sec = second;

    // ת��Ϊϵͳʱ��
    return  mktime(&tm_date);
}

/**
 * ������˾����ʱ���ʽת��Ϊϵͳʱ��
 * @input       strTime     YYYY-MM-DD HH:MM:SS
 */
time_t fundTimetoUnixTime(const string& strTime)
{
    // ȡ�ꡢ�¡��ն�
    int year=0, month=0, day=0, hour=0, minute=0, second=0;
    sscanf(strTime.c_str(), "%04d%02d%02d%02d%02d%02d", &year, &month, &day, &hour, &minute, &second);

    // ������С��1900������0
    if(year < 1900)     return 0;

    // ת��Ϊ����ʱ��
    struct  tm tm_date;
    memset(&tm_date, 0, sizeof(tm));

    tm_date.tm_year =  year - 1900;
    tm_date.tm_mon = month - 1;
    tm_date.tm_mday = day;
    tm_date.tm_hour = hour;
    tm_date.tm_min = minute;
    tm_date.tm_sec = second;

    // ת��Ϊϵͳʱ��
    return  mktime(&tm_date);
}


/**
 * @input YYYYMMDD
 * @output YYYY-MM-DD
 */
string toDate(const string& strDate)
{
    int year, month, day;
    sscanf(strDate.c_str(), "%04d%02d%02d", &year, &month, &day);

    char szTmp[11];
    snprintf(szTmp, sizeof(szTmp), "%04d-%02d-%02d", year, month, day);

    return szTmp;
}

/**
 * ��ȡϵͳʱ��: YYYY-MM-DD HH:MM:SS
 */
string getSysTime()
{
    return getSysTime(time(NULL));
}

/**
 * ��ȡϵͳʱ��: YYYY-MM-DD HH:MM:SS
 */
string getSysTime(time_t t)
{
    struct  tm tm_now;
    localtime_r(&t, &tm_now);

    char szTmp[256];
    snprintf(szTmp, sizeof(szTmp), "%04d-%02d-%02d %02d:%02d:%02d",
                tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);

    return szTmp;
}

/**
 * ��ȡϵͳʱ��: YYYYMMDD
 */
string getSysDate()
{
    time_t t = time(NULL);
    struct  tm tm_now;
    localtime_r(&t, &tm_now);

    char szTmp[256];
    snprintf(szTmp, sizeof(szTmp), "%04d%02d%02d", tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday);

    return szTmp;
}

/**
 * ��ȡϵͳʱ��: YYYYMMDD
 */
string getSysDate(time_t t)
{
    struct  tm tm_now;
    localtime_r(&t, &tm_now);

    char szTmp[256];
    snprintf(szTmp, sizeof(szTmp), "%04d%02d%02d", tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday);

    return szTmp;
}

int getToday()
{
	time_t today = time(NULL);
	time_t tm_result;
	struct tm tm_now;
	localtime_r(&today, &tm_now);

	tm_now.tm_sec = 0;
	tm_now.tm_min = 0;
	tm_now.tm_hour = 0;

	tm_result = mktime(&tm_now);
	//begin = tm_result;
	//end = tm_result + 86400 - 1;
	return tm_result;
}

int GetYesterday()
{
	time_t yesterday = time(NULL) - 24*3600;
	time_t tm_result;
	struct tm tm_now;
	localtime_r(&yesterday, &tm_now);

	tm_now.tm_sec = 0;
	tm_now.tm_min = 0;
	tm_now.tm_hour = 0;

	tm_result = mktime(&tm_now);
	//begin = tm_result;
	//end = tm_result + 86400 - 1;
	return tm_result;
}

int getStartTime(time_t tnow)
{
	time_t tm_result;
	struct tm tm_now;
	localtime_r(&tnow, &tm_now);

	tm_now.tm_sec = 0;
	tm_now.tm_min = 0;
	tm_now.tm_hour = 0;

	tm_result = mktime(&tm_now);
	//begin = tm_result;
	//end = tm_result + 86400 - 1;
	return tm_result;
}

/**
 * ȡʱ����겿��
 *@input:   str   YYYY-MM-DD HH:MM:SS
 */
int year(const string& str)
{
    int year, month, day;
    sscanf(str.c_str(), "%04d-%02d-%02d", &year, &month, &day);
    return year;
}

/**
 * ȡʱ����²���
 *@input:   str   YYYY-MM-DD HH:MM:SS
 */
int month(const string& str)
{
    int year, month, day;
    sscanf(str.c_str(), "%04d-%02d-%02d", &year, &month, &day);
    return month;
}

/**
 * ȡʱ����ղ���
 *@input:   str   YYYY-MM-DD HH:MM:SS
 */
int day(const string& str)
{
    int year, month, day;
    sscanf(str.c_str(), "%04d-%02d-%02d", &year, &month, &day);
    return day;
}

/**
 * ȡ��ǰ����
 *@output:   string   YYYYMMDD
 */
string nowdate(const string& str)
{
    char szTmp[9];    //��������
    memset(szTmp, 0, sizeof(szTmp));

    int year, month, day;
    sscanf(str.c_str(), "%04d-%02d-%02d", &year, &month, &day);

    snprintf(szTmp, sizeof(szTmp), "%04d%02d%02d", year, month, day);

    return szTmp;
}

/**
 * ȡ��һ������
 *@output:   string   YYYYMM
 */
string lastmonth(const string& str)
{
    char szTmp[9];    //��������
    int year, month;

    memset(szTmp, 0, sizeof(szTmp));

    year = atoi(str.substr(0, 4).c_str());
    month = atoi(str.substr(4, 2).c_str());

    month-=1;
    if (month == 0)
    {
        month = 12;
        year-=1;
    }

    snprintf(szTmp, sizeof(szTmp), "%04d%02d", year, month);
    return szTmp;
}

/**
 * ȡ��һ������
 *@output:   string   YYYYMM
 */
string nextdate(const string& str)
{
    char szTmp[9];    //��������
    memset(szTmp, 0, sizeof(szTmp));

    int year, month;

    year = atoi(str.substr(0, 4).c_str());
    month = atoi(str.substr(4, 2).c_str());
    /**
     * monthΪ�·ݣ�ȡֵ[1,12]
     * �˴�������һ��ʱmonth�ȼ�1,�ټ�1,Ȼ����ģ12���˴�month-1+1дΪmonth
     * ģ12�ķ�ΧΪ[0,11],��������Ҫ��1���·�ֵ�ָ���[1,12]
     */
    month = month%12 + 1;
    if (month == 1)
    {
        year+=1;
    }

    snprintf(szTmp, sizeof(szTmp), "%04d%02d", year, month);
    return szTmp;
}

/**
 * ���ַ����е�a�ַ��滻Ϊb�ַ�
 */
char* replace(char* str, char a, char b)
{
    std::replace(str, str + strlen(str), a, b);

    return str;
}

/**
 * ����Ƿ�Ϊ����
 */
int isNumString(const char *str)
{
    const char * p = str;

    if (p == NULL)
    {
        return 0;
    }

    while (*p != '\0')
    {
        if (! isdigit(*p))
        {
            return 0;
        }
        p++;
    }

    return 1;
}

/**
 * ָ��ת��
 */
const char *ValiStr(char *str)
{
        if (str == NULL)
                return "";
        else
                return str;
}
/**
 * ָ��ת��
 */
const char *ValiDateStr(char *str)
{
        if (str == NULL)
                return "";
        else if(strcmp(str,"0000-00-00 00:00:00")==0)
                return "";
        else
                return str;
}

/**
 *�ַ����۳���������
 */
void split(vector <int > & list, const char * sp)
{
    char szSource[1024];
    snprintf(szSource, sizeof(szSource), "%s", sp);
    char * p = (char *)szSource;
    int iLen = strlen(sp);

    for(int iEnd = 0;iEnd < iLen; iEnd ++)
    {
        if(szSource[iEnd] == '|')
        {
            szSource[iEnd] = '\0';


            int itmp = atoi(p);
            if(itmp != 0)
            {
                list.push_back(itmp);
            }
            p = szSource + iEnd + 1;
        }
    }

    int itmp = atoi(p);
    if(itmp != 0)
    {
       list.push_back(itmp);
    }
    return ;
}

/**
 * �ַ����ָ����. ��splitter�и��ַ���
 * �ַ����ǿ�ʱ���ؿ�vector
 */
vector<string> split(const string &src, const char* splitter)
{
    vector<string> strv;
    string::size_type pos = 0, endpos = 0;
    string::size_type len = strlen(splitter);

    while(pos != string::npos && pos < src.length())
    {
        endpos = src.find(splitter, pos, len);

        string item;

        if(endpos != string::npos)
        {
            item = src.substr(pos, endpos - pos);
            pos = endpos + len;
        }
        else
        {
            item = src.substr(pos);
            pos = string::npos;
        }

        strv.push_back(item);
    }

    return strv;
}

//���� chΪ�ָ����з�src,���зֺ�����ݷŵ�strv��
void split_ex(const string &src, const char ch, vector<string>& strv)
{
	//vector<string> strv;
	string::size_type pos = 0, endpos = 0;
	//string::size_type len = strlen(splitter);

	while (pos != string::npos && pos < src.length())
	{
		//endpos = src.find(splitter, pos, len);
		endpos = src.find(ch, pos);

		string item;

		if (endpos != string::npos)
		{
			item = src.substr(pos, endpos - pos);
			pos = endpos + 1;
		}
		else
		{
			item = src.substr(pos);
			pos = string::npos;
		}

		strv.push_back(item);
	}
}


/**
 * ���ַ�����׺һ���±�
 */
string add_suffix(const char* name, int n)
{
    char szItem[128];
    snprintf(szItem, sizeof(szItem), "%s%d", name, n);

    return szItem;
}

/**
 * ���ַ���ȥ���ո�
 */
string& strTrim(string& str)
{
    while (string::npos != str.find_first_of(' '))
    {
        str.erase(str.find_first_of(' '), 1);
    }

    return str;
}

/**
 * ���ַ���ȥ���ո������ַ���ֻ�������ֺ���ĸ
 */
string strTrimSpecial(const string& src)
{
    string result;
    string::const_iterator iter;

    for(iter = src.begin(); iter != src.end(); ++iter)
    {
        char ch = *iter;
        if ((ch >= '0' && ch <= '9') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z'))
        {
            result += ch;
        }
    }

    return result;
}

int monthInterval(const string& s_time , const string& e_time)
{
    int s_year = year(s_time) ;
    int s_month = month(s_time) ;
    int e_year = year(e_time) ;
    int e_month = month(e_time) ;

    return 12 * (e_year - s_year) + (e_month - s_month) + 1 ;
}

string str_replace(string rawStr,string from,string to)
{
    if (from == "")
    {
        return rawStr;
    }
    size_t pos = rawStr.find(from);
    string retstr;
    for (;pos != string::npos;)
    {
        retstr +=rawStr.substr(0,pos) + to;
        if (pos+from.length() < rawStr.length())
        {
            rawStr = rawStr.substr(pos+from.length(),rawStr.length()-pos-from.length());
        }
        else
        {
            rawStr="";
        }
        pos = rawStr.find(from);
    }
    retstr += rawStr;
    return retstr;
};


static inline bool isspace(char ch)
{
    return (ch == ' ' || ch == '\t');
}

void InplaceTrimLeft(std::string& strValue)
{
    size_t pos = 0;
    for (size_t i = 0; i < strValue.size(); ++i)
    {
        if (isspace((unsigned char)strValue[i]))
            ++pos;
        else
            break;
    }
    if (pos > 0)
        strValue.erase(0, pos);
}

void InplaceTrimRight(std::string& strValue)
{
    size_t n = 0;
    for (size_t i = 0; i < strValue.size(); ++i)
    {
        if (isspace((unsigned char)strValue[strValue.length() - i - 1]))
            ++n;
        else
            break;
    }
    if (n != 0)
        strValue.erase(strValue.length() - n);
}

void InplaceTrim(std::string& strValue)
{
    InplaceTrimRight(strValue);
    InplaceTrimLeft(strValue);
}


void str_split(
    const std::string& strMain,
    char chSpliter,
    std::vector<std::string>& strList,
    bool bReserveNullString)
{
    strList.clear();

    if (strMain.empty())
        return;

    size_t nPrevPos = 0;
    size_t nPos;
    std::string strTemp;
    while ((nPos = strMain.find(chSpliter, nPrevPos)) != string::npos)
    {
        strTemp.assign(strMain, nPrevPos, nPos - nPrevPos);
        InplaceTrim(strTemp);
        if (bReserveNullString || !strTemp.empty())
            strList.push_back(strTemp);
        nPrevPos = nPos + 1;
    }

    strTemp.assign(strMain, nPrevPos, strMain.length() - nPrevPos);
    InplaceTrim(strTemp);
    if (bReserveNullString || !strTemp.empty())
        strList.push_back(strTemp);
}


//int CopyFile(const char *SourceFile,const char *NewFile)
//{
//	std::ifstream in;
//	std::ofstream out;
//	in.open(SourceFile,std::ios::binary);//��Դ�ļ�
//	if(in.fail())//��Դ�ļ�ʧ��
//	{
//	   in.close();
//	   return 0;
//	}
//	out.open(NewFile,std::ios::binary);//����Ŀ���ļ�
//	if(out.fail())//�����ļ�ʧ��
//	{
//	   out.close();
//	   in.close();
//	   return 0;
//	}
//	else//�����ļ�
//	{
//	   out<<in.rdbuf();
//	   out.close();
//	   in.close();
//	   return 1;
//	}
//}

int file_mmap(const std::string& map_file_name, vector<std::string>& vecBill)
{
	void *memory = NULL;
	int file_length = 0;
	int fd = open(map_file_name.c_str(), O_RDONLY);
	if (fd < 0)
	{
		printf("file open [%s] error\n", map_file_name.c_str());
		return -1;
	}
	file_length = lseek(fd, 1, SEEK_END);
	memory = mmap(NULL, file_length, PROT_READ, MAP_SHARED, fd, 0);
	//printf("memory = %s\n", (char *)memory);
	split_ex((char *)memory, '\n', vecBill);
	/*for (int i = 0; i < vecBill.size(); ++i)
	{
		printf("i = %d value= %s\n", i, vecBill[i].c_str());
	}*/
	close(fd);
	munmap(memory, file_length);
	return 0;
}

void AddJsonMap(JsonMap& map,const char* key,const std::string& value)
{
	map.insert(JsonMap::value_type(JsonType(key), JsonType(value)));

}

