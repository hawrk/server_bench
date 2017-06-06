/*
 * CSyabse.h
 *
 *  Created on: 2017年5月27日
 *      Author: hawrkchen
 */

#ifndef _CSYABSE_H_
#define _CSYABSE_H_

#include <iostream>
#include <string.h>
#include <stdio.h>

#define CSYBASE_SLEN_NORM 64
#define CSYBASE_SLEN_LONG 256

class CSyBase
{
public:
	CSyBase();
	virtual ~CSyBase();

public:
    char ms_erro[CSYBASE_SLEN_LONG];
    char ms_host[CSYBASE_SLEN_NORM];
    char ms_user[CSYBASE_SLEN_NORM];
    char ms_pass[CSYBASE_SLEN_NORM];
    char ms_db[CSYBASE_SLEN_NORM];

    int             mi_conn;

	int init(const char *as_host,
				const char *as_user,
				const char *as_pass,
				const char *as_db = NULL );

};




#endif /* _CSYABSE_H_ */
