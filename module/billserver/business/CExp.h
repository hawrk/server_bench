#ifndef _CEXP_H_HEADER_INCLUDED_
#define _CEXP_H_HEADER_INCLUDED_

#include <stdio.h>
#include <string>

using namespace std;

class CTrsExp
{
  public:
    string retcode, retmsg;
    CTrsExp ( const char *str )
    {
        retmsg = string ( str );
        retcode = string ( "0" );
    };

    CTrsExp ( string str )
    {
        retmsg = string ( str );
        retcode = string ( "0" );
    };
    CTrsExp ( int cd, string str )
    {
        char szCd[32] = {0};
        
        sprintf(szCd , "%04d", cd);
        retmsg = str;
        retcode = string(szCd);
    };
    CTrsExp ( string cd, string str )
    {
        retmsg = str;
        retcode = cd;
    };
};


#endif
