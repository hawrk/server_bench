/*
 * BankFactory.h
 *
 *  Created on: 2017年6月2日
 *      Author: hawrkchen
 */

#ifndef _BANKFACTORY_H_
#define _BANKFACTORY_H_

#include <string>

#ifdef _SYBASE
#include "CBillConstrastABC.h"
#else
#include "CBillConstrastCommon.h"
#include "CBillConstrastSPDB.h"
#endif
#include "CBillConstrastBase.h"


class CBankFactory
{
public:
	CBankFactory();
	virtual ~CBankFactory();

	virtual CBillContrastBase* CreateBankFactory(const std::string& bank_type);

private:

};



#endif /* _BANKFACTORY_H_ */
