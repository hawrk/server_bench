/*
 * gzipbase64.cpp
 *
 *  Created on: 2017年8月11日
 *      Author: hawrkchen
 */

#include "gzipbase64.h"

#include <stdlib.h>

#include <iostream>
#include <sstream>
#include <vector>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zlib.hpp>

using namespace std;

/*
LastUpdate:2015-03-16 by kagula
《How to compress and encode data with zlib and base64》
http://www.koboldtouch.com/display/IDCAR/How+to+compress+and+encode+data+with+zlib+and+base64

How to compile boost zlib library?
download zlib from http://www.zlib.net
in vs2010 console prompt
set ZLIB_SOURCE="D:\SDK\zlib-1.2.8"
D:\SDK\boost_1_55_0>bjam --toolset=msvc-10.0 --with-iostreams
*/

// Base 64 encoding, updated and corrected - original code from:
// http://stackoverflow.com/a/6782480/201863
unsigned char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int mod_table[] = {0, 2, 1};

char* base64_encode(const unsigned char* data, size_t input_length, size_t* output_length)
{
    *output_length = ((input_length - 1) / 3) * 4 + 4;

    char* encoded_data = (char*)(malloc(*output_length+1));
    encoded_data[*output_length]=0;
    if (encoded_data == NULL)
    {
        return NULL;
    }

    for (size_t i = 0, j = 0; i < input_length;)
    {
        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
    {
        encoded_data[*output_length - 1 - i] = '=';
    }

    return encoded_data;
}

static char *decoding_table = NULL;

void build_decoding_table() {

    decoding_table = (char *)malloc(256);

    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}

const static char padCharacter = '=';
std::vector<char> base64Decode(const std::basic_string<char>& input)
{
    if (input.length() % 4) //Sanity check
        throw std::runtime_error("Non-Valid base64!");
    size_t padding = 0;
    if (input.length())
    {
        if (input[input.length()-1] == padCharacter)
            padding++;
        if (input[input.length()-2] == padCharacter)
            padding++;
    }
    //Setup a vector to hold the result
    std::vector<char> decodedBytes;
    decodedBytes.reserve(((input.length()/4)*3) - padding);
    unsigned int temp=0; //Holds decoded quanta
    std::basic_string<char>::const_iterator cursor = input.begin();
    while (cursor < input.end())
    {
        for (size_t quantumPosition = 0; quantumPosition < 4; quantumPosition++)
        {
            temp <<= 6;
            if       (*cursor >= 0x41 && *cursor <= 0x5A) // This area will need tweaking if
                temp |= *cursor - 0x41;                   // you are using an alternate alphabet
            else if  (*cursor >= 0x61 && *cursor <= 0x7A)
                temp |= *cursor - 0x47;
            else if  (*cursor >= 0x30 && *cursor <= 0x39)
                temp |= *cursor + 0x04;
            else if  (*cursor == 0x2B)
                temp |= 0x3E; //change to 0x2D for URL alphabet
            else if  (*cursor == 0x2F)
                temp |= 0x3F; //change to 0x5F for URL alphabet
            else if  (*cursor == padCharacter) //pad
            {
                switch( input.end() - cursor )
                {
                case 1: //One pad character
                    decodedBytes.push_back((temp >> 16) & 0x000000FF);
                    decodedBytes.push_back((temp >> 8 ) & 0x000000FF);
                    return decodedBytes;
                case 2: //Two pad characters
                    decodedBytes.push_back((temp >> 10) & 0x000000FF);
                    return decodedBytes;
                default:
                    throw std::runtime_error("Invalid Padding in Base 64!");
                }
            }  else
                throw std::runtime_error("Non-Valid Character in Base 64!");
            cursor++;
        }
        decodedBytes.push_back((temp >> 16) & 0x000000FF);
        decodedBytes.push_back((temp >> 8 ) & 0x000000FF);
        decodedBytes.push_back((temp      ) & 0x000000FF);
    }
    return decodedBytes;
}
/*
gzipbase64函数等价java下面这个函数
public static String gzip(String primStr)
{
if (primStr == null || primStr.length() == 0)
{
return primStr;
}
ByteArrayOutputStream out = new ByteArrayOutputStream();
GZIPOutputStream gzip = null;
try
{
gzip = new GZIPOutputStream(out);
gzip.write(primStr.getBytes());
}
catch (IOException e)
{
e.printStackTrace();
}
finally
{
if (gzip != null)
{
try
{
gzip.close();
}
catch (IOException e)
{
e.printStackTrace();
}
}
}
return new sun.misc.BASE64Encoder().encode(out.toByteArray());
}
可以用下面这个函数解密
public static String gunzip(String compressedStr)
{
if (compressedStr == null)
{
return null;
}
ByteArrayOutputStream out = new ByteArrayOutputStream();
ByteArrayInputStream in = null;
GZIPInputStream ginzip = null;
byte[] compressed = null;
String decompressed = null;
try
{
compressed = new sun.misc.BASE64Decoder().decodeBuffer(compressedStr);
in = new ByteArrayInputStream(compressed);
ginzip = new GZIPInputStream(in);
byte[] buffer = new byte[1024];
int offset = -1;
while ((offset = ginzip.read(buffer)) != -1)
{
out.write(buffer, 0, offset);
}
decompressed = out.toString();
}
catch (IOException e)
{
e.printStackTrace();
}
finally
{
if (ginzip != null)
{
try
{
ginzip.close();
}
catch (IOException e)
{
}
}
if (in != null)
{
try
{
in.close();
}
catch (IOException e)
{
}
}
if (out != null)
{
try
{
out.close();
}
catch (IOException e)
{
}
}
}
return decompressed;
}
*/
std::string gzipbase64( std::string strIn)
{
    using namespace std;

    istringstream iss(strIn);
    stringstream ss;
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(boost::iostreams::gzip_compressor());
    in.push(iss);
    boost::iostreams::copy(in, ss);

    size_t output_length;
    char * outBuf = base64_encode((const unsigned char *)(ss.str().c_str()),ss.str().length(),&output_length);

    std::string strResult;
    strResult.append(outBuf);
    free(outBuf);
    return strResult;
}

std::string zlibbase64( std::string strIn )
{
    using namespace std;

    istringstream iss(strIn);
    stringstream ss;
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(boost::iostreams::zlib_compressor());
    in.push(iss);
    boost::iostreams::copy(in, ss);

    size_t output_length;
    char * outBuf = base64_encode((const unsigned char *)(ss.str().c_str()),ss.str().length(),&output_length);

    std::string strResult;
    strResult.append(outBuf);
    free(outBuf);
    return strResult;
}

void ungzip(const std::vector<char> &compressed,
    std::vector<char> &decompressed)
{
    namespace io = boost::iostreams;
    io::filtering_ostream os;

    os.push(io::gzip_decompressor());
    os.push(io::back_inserter(decompressed));

    io::write(os, &compressed[0], compressed.size());
}
/*
在java端使用“sun.misc.BASE64Encoder().encode”字符串当中会有“\r\n”。
cipherJava = cipherJava.replaceAll("\r\n", "");
*/
std::string decode_gzipbase64( std::string strIn )
{
    vector<char> vecSrc =  base64Decode(strIn);

    vector<char> vecDst;
    ungzip(vecSrc,vecDst);

    string dst;
    for (size_t i=0;i<vecDst.size();i++)
    {
        dst.push_back(vecDst[i]);
    }

    return dst;
}


