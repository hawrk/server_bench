#include "urlparammap.h"
#include <algorithm>
#include <stdio.h>

/**
 * ȱʡ�ָ���ת���ַ�
 */
const char DFL_SEG_DEL = '&';   
const char DFL_ELE_DEL = '=';

/**
* ������ڵ�һλ����Ϣ
*/
const char* __MUST_IN_FIRST_FIELD = "result";

inline char hextochar(char c1, char c2)
{
    char tmp[] = "0x00";
    int val;

    tmp[2]=c1;
    tmp[3]=c2;
    sscanf(tmp, "%x", &val);

    return static_cast<char>(val);
}

/**
 * ���徲̬��Ա����
 */
char UrlParamMap::__SEG_DEL = DFL_SEG_DEL ; // �ηָ���
char UrlParamMap::__ELE_DEL = DFL_ELE_DEL; // ����-ֵ�ָ���
PFUN_ESCAPE UrlParamMap::__PFUN_ESCAPE = UrlParamMap::encodeUrl; // ת�庯��ָ�� 
PFUN_UNESCAPE UrlParamMap::__PFUN_UNESCAPE = UrlParamMap::decodeUrl; // ת�庯��ָ�� 

/**
 * ȱʡurlת�庯��
 *@input: str ��ת�崮
 *@return: ת���Ĵ�
 */
string UrlParamMap::encodeUrl(const string& str)
{
    static const char* chTable[] = {
                    "%00","%01","%02","%03","%04","%05","%06","%07","%08","%09",
                    "%0a","%0b","%0c","%0d","%0e","%0f","%10","%11","%12","%13",
                    "%14","%15","%16","%17","%18","%19","%1a","%1b","%1c","%1d",
                    "%1e","%1f"," ","%21","%22","%23","%24","%25","%26","%27",
                    "(",")","*","%2b",",","-",".","%2f","0","1",
                    "2","3","4","5","6","7","8","9",":","%3b",
                    "%3c","%3d","%3e","%3f","%40","A","B","C","D","E",
                    "F","G","H","I","J","K","L","M","N","O",
                    "P","Q","R","S","T","U","V","W","X","Y",
                    "Z","[","\\","]","%5e","_","%60","a","b","c",
                    "d","e","f","g","h","i","j","k","l","m",
                    "n","o","p","q","r","s","t","u","v","w",
                    "x","y","z","%7b","|","%7d","~","%7f","%80","%81",
                    "%82","%83","%84","%85","%86","%87","%88","%89","%8a","%8b",
                    "%8c","%8d","%8e","%8f","%90","%91","%92","%93","%94","%95",
                    "%96","%97","%98","%99","%9a","%9b","%9c","%9d","%9e","%9f",
                    "%a0","%a1","%a2","%a3","%a4","%a5","%a6","%a7","%a8","%a9",
                    "%aa","%ab","%ac","%ad","%ae","%af","%b0","%b1","%b2","%b3",
                    "%b4","%b5","%b6","%b7","%b8","%b9","%ba","%bb","%bc","%bd",
                    "%be","%bf","%c0","%c1","%c2","%c3","%c4","%c5","%c6","%c7",
                    "%c8","%c9","%ca","%cb","%cc","%cd","%ce","%cf","%d0","%d1",
                    "%d2","%d3","%d4","%d5","%d6","%d7","%d8","%d9","%da","%db",
                    "%dc","%dd","%de","%df","%e0","%e1","%e2","%e3","%e4","%e5",
                    "%e6","%e7","%e8","%e9","%ea","%eb","%ec","%ed","%ee","%ef",
                    "%f0","%f1","%f2","%f3","%f4","%f5","%f6","%f7","%f8","%f9",
                    "%fa","%fb","%fc","%fd","%fe","%ff"};

    string result;
    for(string::const_iterator it= str.begin(); it!=str.end(); ++it)
    {
        result += chTable[static_cast<unsigned char>(*it)];
    }

    return result;
}

/**
 * ȱʡurl��ת�庯��
 *@input: str ת�崮
 *@return: ת��ǰ�Ĵ�
 */
string UrlParamMap::decodeUrl(const string& str)
{
    string result;
  
    for(string::const_iterator it= str.begin(); it!=str.end(); ++it)
    {
        if (*it ==  '%' && std::distance(it, str.end()) >= 3)
        {
            result.append(1, hextochar(*(it+1), *(it+2)));
            std::advance(it, 2);
        }
        else 
        {
            result.append(1, *it);
        }
    }
    
    return result;
}

/**
 * ���캯��
 */
UrlParamMap::UrlParamMap(const string& str)
{
    parseUrl(str);
}

/**
 * ����URL������
 */
/*
void UrlParamMap::parseUrl(const string& strUrl)
{
    map<string, string>::clear();

    string::size_type q = 0;
    string::size_type p = 0;
    
    while(q != strUrl.npos)
    {
        p = strUrl.find(__SEG_DEL, q);
        
        parseNV(strUrl, q, p);

        q = (p == strUrl.npos) ? strUrl.npos : ++p;
    }
}*/
    
/**
 * �������map
 */
void UrlParamMap::packUrl(string& result)
{
    result.clear();

    iterator tmp = find(__MUST_IN_FIRST_FIELD);

    if (tmp != end())
    {
        result += tmp->first;
        result += __ELE_DEL;
        result += (*__PFUN_ESCAPE)(tmp->second);
        result += __SEG_DEL;
    }

    for(iterator it = begin(); it != end(); ++it)
    {
        if (it->first == __MUST_IN_FIRST_FIELD)
        {
            continue;
        }
    
        result += it->first;
        result += __ELE_DEL;
        result += (*__PFUN_ESCAPE)(it->second);
        result += __SEG_DEL;
    }
}

/**
 * ����һ��Name-value��
 */
void UrlParamMap::parseNV(const string& strUrl, string::size_type beg, string::size_type end)
{
    string::size_type pos = strUrl.find(__ELE_DEL, beg);

    if(pos != string::npos && pos <= end)
    {
        map<string, string>::value_type item(strUrl.substr(beg, pos-beg), (*__PFUN_UNESCAPE)(strUrl.substr(pos+1, end - pos - 1)));
        
        map<string, string>::insert(item);
    }
}


void UrlParamMap::parseUrl(const string& strUrl)
{
	map<string, string>::clear();

	string::size_type beg = 0;
	string::size_type end = 0;

	while (beg != strUrl.npos)
	{
		end = strUrl.find(__SEG_DEL, beg);

		string::size_type pos = strUrl.find(__ELE_DEL, beg);

		if (pos != string::npos && pos <= end)
		{
			std::string key = strUrl.substr(beg, pos - beg);
			std::string strValue = strUrl.substr(pos + 1, end - pos - 1);
			char szDecodeValue[10240];
			clib_urldecode_comm(strValue.c_str(), szDecodeValue, sizeof(szDecodeValue));
			map<string, string>::insert(std::make_pair(key, szDecodeValue));
		}

		beg = (end == strUrl.npos) ? strUrl.npos : ++end;
	}
}


int UrlParamMap::clib_urldecode_comm(const char *as_src, char *as_des, const int ai_len)
{
	int  i = 0, j = 0;
	long l_asc = 0;
	char s_buf[10240] = { 0 };
	char s_tmp[10240] = { 0 };

	if (as_src == NULL
		|| as_des == NULL
		|| ai_len <= 0
		|| ai_len > 10240) {
		return(-1);
	} // if

	i = 0;
	j = 0;
	memset(s_buf, 0, sizeof(s_buf));
	while ((i < ai_len) && (as_src[i] != '\0') && (j < (int)sizeof(s_buf)-1)) {
		switch (as_src[i]) {
		case '%':
			snprintf(s_tmp, 3, "%s", as_src + i + 1);
			l_asc = strtol(s_tmp, NULL, 16);
			//s_buf[j] = toascii( l_asc );
			s_buf[j] = (char)l_asc;
			i += 2;
			break;
		case '+':
			s_buf[j] = ' ';
			break;
		default:
			s_buf[j] = as_src[i];
			break;
		} // switch

		i++;
		j++;
	} // while

	snprintf(as_des, ai_len, "%s", s_buf);
	return 0;
}