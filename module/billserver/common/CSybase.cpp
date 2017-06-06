/*
 * CSyabse.cpp
 *
 *  Created on: 2017年5月27日
 *      Author: hawrkchen
 */

#include "CSybase.h"


CSyBase::CSyBase()
{
	//TODO:
	mi_conn = 0;
}

CSyBase::~CSyBase()
{
	//TODO:
}


int CSyBase::init(const char *as_host,
						const char *as_user,
						const char *as_pass,
						const char *as_db)
{
	snprintf( ms_host, sizeof(ms_host), "%s", as_host );
    snprintf( ms_user, sizeof(ms_user), "%s", as_user );
    snprintf( ms_pass, sizeof(ms_pass), "%s", as_pass );
    snprintf( ms_db,   sizeof(ms_db),   "%s", as_db );
    mi_conn     = 0;

	return 0;
};
