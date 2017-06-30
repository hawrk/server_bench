/*
 * BankFactory.cpp
 *
 *  Created on: 2017年6月2日
 *      Author: hawrkchen
 */
#include "BankFactory.h"

CBankFactory::CBankFactory()
{
	//TODO:
}

CBankFactory::~CBankFactory()
{
	//TODO:
}


CBillContrastBase* CBankFactory::CreateBankFactory(const std::string& bank_type)
{
#ifdef _SYBASE
	if(bank_type == "ABC")  //农行
	{
		return new CBillContrastABC();
	}
#else
	if(bank_type == "tarbc")
	{
		return new CBillContrastCommon();
	}
	if(bank_type == "SPDB")
	{
		return new CBillContrastSPDB();
	}
#endif
//	if(bank_type == "TARCB")  //泰山农商行
//	{
//		return new CTARCBBillContrast();
//	}

	return NULL;
}




