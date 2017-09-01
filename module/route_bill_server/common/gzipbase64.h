/*
 * gzipbase64.h
 *
 *  Created on: 2017年8月11日
 *      Author: hawrkchen
 */

#ifndef _GZIPBASE64_H_
#define _GZIPBASE64_H_

#include <string>

std::string gzipbase64(std::string strIn);
std::string decode_gzipbase64(std::string strIn);
std::string zlibbase64(std::string strIn);


#endif /* _GZIPBASE64_H_ */
