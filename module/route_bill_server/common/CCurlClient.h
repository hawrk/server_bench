#ifndef __CurlClient__
#define __CurlClient__

#include <curl/curl.h>
#include <string>

using std::string;

class CurlClient
{
public:
    static size_t WriteData(char *data, size_t block_size, size_t block_count, string *response)
    {
        if(data == NULL) return 0;

        size_t len = block_size * block_count;
        response->append(data, len);
        return len;
    }

    static size_t WriteFile(char *data, size_t block_size, size_t block_count, FILE *file)
    {
        if(data == NULL) return 0;

        size_t len = block_size * block_count;
        fwrite(data, len, 1, file);
        return len;
    }

    CurlClient():pcurl_(NULL), timeout_(10), sslverify_(true), pchunk_(NULL)
    {
        pcurl_ = curl_easy_init();
        certfile_ = "";
        certpwd_ = "";
        certtype_ = "";
    }

    ~CurlClient()
    {
        curl_easy_cleanup(pcurl_);
    }

    void SetTimeOut(int timeout)
    {
        timeout_ = timeout;
    }

    void SetSSLVerify(bool verify)
    {
        sslverify_ = verify;
    }

    void SetSSLCert(const string & certpath, const string & certpwd, const string & certtype = "PEM")
    {
        certfile_ = certpath;
        certpwd_ = certpwd;
        certtype_ = certtype;
    }

    void SetRedirect(bool redirect, int redirect_times=10)
    {
        redirect_ = redirect;
        redirect_times_ = redirect_times;
    }

    void Reset()
    {
        timeout_   = 10;
        sslverify_ = true;
        certfile_ = "";
        certpwd_ = "";
        certtype_ = "";
        redirect_ = false;
        redirect_times_ = 0;
        errcode_   = 0;
        errinfo_   = "";
        curl_easy_reset(pcurl_);
    }

    void SetCookie(const string& cookie)
    {
        curl_easy_setopt(pcurl_, CURLOPT_COOKIE, cookie.c_str());
    }

    void SetHeader(const string& header)
    {
        pchunk_ = curl_slist_append(pchunk_, header.c_str());
    }

    string GetErrInfo()
    {
        return errinfo_;
    }
    int GetErrCode()
    {
        return errcode_;
    }

    bool Get(const string& url, string& respbody)
    {
        string resphead;
        return Get(url, resphead, respbody);
    }

    bool Get(const string& url, string& resphead, string& respbody)
    {
        return Request(url, "", "get", resphead, respbody);
    }

    bool Put(const string& url, const string& request, string& respbody)
    {
        string resphead;
        return Put(url, request, resphead, respbody);
    }

    bool Put(const string& url, const string& request,
                string& resphead, string& respbody)
    {
        return Request(url, request, "put", resphead, respbody);
    }

    bool Post(const string& url, const string& request, string& respbody)
    {
        string resphead;
        return Post(url, request, resphead, respbody);
    }

    bool Post(const string& url, const string& request,
                string& resphead, string& respbody)
    {
        return Request(url, request, "post", resphead, respbody);
    }

    bool Delete(const string& url, string& respbody)
    {
        string resphead;
        return Delete(url, resphead, respbody);
    }

    bool Delete(const string& url, string& resphead, string& respbody)
    {
        return Request(url, "", "delete", resphead, respbody);
    }

    bool Save( const std::string& url, const char *filename)
    {
        respcode_ = 0;

        FILE* file = fopen(filename, "wb");

        //curl_easy_reset(pcurl_);
        curl_easy_setopt(pcurl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(pcurl_, CURLOPT_WRITEFUNCTION, WriteFile);
        curl_easy_setopt(pcurl_, CURLOPT_WRITEDATA, file);
        curl_easy_setopt(pcurl_, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(pcurl_, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(pcurl_, CURLOPT_TIMEOUT, timeout_);
        curl_easy_setopt(pcurl_, CURLOPT_CONNECTTIMEOUT, timeout_);
        if (!sslverify_)
        {
            curl_easy_setopt(pcurl_, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(pcurl_, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        if(certfile_.size()&&certpwd_.size())
        {
            curl_easy_setopt(pcurl_, CURLOPT_SSLCERT, certfile_.c_str());
            curl_easy_setopt(pcurl_, CURLOPT_SSLCERTPASSWD, certpwd_.c_str());
            curl_easy_setopt(pcurl_, CURLOPT_SSLCERTTYPE, certtype_.c_str());
        }
        if (NULL != pchunk_)
        {
            curl_easy_setopt(pcurl_, CURLOPT_HTTPHEADER, pchunk_);
        }

        CURLcode rc = curl_easy_perform(pcurl_);
        fclose(file);
        if(rc != CURLE_OK)
        {
            errinfo_ =  string(curl_easy_strerror(rc));
            errcode_ =  int(rc);
            return false;
        }

        rc = curl_easy_getinfo(pcurl_, CURLINFO_RESPONSE_CODE , &respcode_);
        if(rc != CURLE_OK)
        {
            errinfo_ =  string(curl_easy_strerror(rc));
            errcode_ =  int(rc);
            return false;
        }

        return true;
    }

private:
    bool Request(const string& url, const string& request, string method,
                string& resphead, string& respbody)
    {
        respcode_ = 0;

        resphead.clear();
        resphead.reserve(4 * 1024);
        respbody.clear();
        respbody.reserve(64 * 1024);

        //curl_easy_reset(pcurl_);
        curl_easy_setopt(pcurl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(pcurl_, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(pcurl_, CURLOPT_WRITEFUNCTION, WriteData);
        curl_easy_setopt(pcurl_, CURLOPT_HEADERDATA, &resphead);
        curl_easy_setopt(pcurl_, CURLOPT_WRITEDATA, &respbody);
        curl_easy_setopt(pcurl_, CURLOPT_TIMEOUT, timeout_);
        curl_easy_setopt(pcurl_, CURLOPT_CONNECTTIMEOUT, timeout_);

        if (!sslverify_)
        {
            curl_easy_setopt(pcurl_, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(pcurl_, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        if(certfile_.size()&&certpwd_.size())
        {
            curl_easy_setopt(pcurl_, CURLOPT_SSLCERT, certfile_.c_str());
            curl_easy_setopt(pcurl_, CURLOPT_SSLCERTPASSWD, certpwd_.c_str());
            curl_easy_setopt(pcurl_, CURLOPT_SSLCERTTYPE, certtype_.c_str());
        }
        if(redirect_&&redirect_times_)
        {
            curl_easy_setopt(pcurl_, CURLOPT_FOLLOWLOCATION, true);
            curl_easy_setopt(pcurl_, CURLOPT_MAXREDIRS, redirect_times_);
        }
        if (NULL != pchunk_)
        {
            curl_easy_setopt(pcurl_, CURLOPT_HTTPHEADER, pchunk_);
        }
        if(method == "get")
        {
            curl_easy_setopt(pcurl_, CURLOPT_HTTPGET, 1);
        }
        else if(method == "post")
        {
            curl_easy_setopt(pcurl_, CURLOPT_POST, 1L);
            curl_easy_setopt(pcurl_, CURLOPT_POSTFIELDSIZE, request.length());
            curl_easy_setopt(pcurl_, CURLOPT_POSTFIELDS, request.c_str());
        }
        else if(method == "put")
        {
            curl_easy_setopt(pcurl_, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(pcurl_, CURLOPT_POSTFIELDSIZE, request.length());
            curl_easy_setopt(pcurl_, CURLOPT_POSTFIELDS, request.c_str());
        }
        else if(method == "delete")
        {
            curl_easy_setopt(pcurl_, CURLOPT_CUSTOMREQUEST, "DELETE");
        }
        else
        {
            return false;
        }

        CURLcode rc = curl_easy_perform(pcurl_);
        resphead.reserve(resphead.size());
        respbody.reserve(respbody.size());
        if(rc != CURLE_OK)
        {
            errinfo_ =  string(curl_easy_strerror(rc));
            errcode_ =  int(rc);
            return false;
        }

        rc = curl_easy_getinfo(pcurl_, CURLINFO_RESPONSE_CODE , &respcode_);
        if(rc != CURLE_OK)
        {
            errinfo_ =  string(curl_easy_strerror(rc));
            errcode_ =  int(rc);
            return false;
        }

        return true;
    }


private:
    CURL*       pcurl_;
    int         timeout_;
    bool        sslverify_;
    bool        redirect_;
    int         redirect_times_;
    int         errcode_;
    string      errinfo_;
    long        respcode_;
    curl_slist* pchunk_;
    string      certfile_;
    string      certpwd_;
    string      certtype_;
};

#endif

