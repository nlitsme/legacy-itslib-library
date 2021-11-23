#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "stringutils.h"

class HttpHeaders {
    // behaves similar to HttpQuery, except the parsing/encoding is different.
public:
    struct keyval {
        keyval(const  std::string& key, const std::string& val)
            : key(key), val(val)
        {}

        std::string key;
        std::string val;
    };
    typedef std::vector<keyval> sslist;

    static bool is_lws(char c)
    {
        return (c=='\r' || c=='\n' || c=='\t' || c==' ');
    }
    // replace LWS by single spaces.
    // todo: should i leave LWS inside quotes alone?
    static std::string sanitize(const std::string& dirty)
    {
        std::string clean;
        bool wrotespace= false;
        auto i= dirty.begin();
        // skip leading LWS
        while (i!=dirty.end() && is_lws(*i))
            i++;
        while (i!=dirty.end())
        {
            char c= *i;
            if (is_lws(c)) {
                if (!wrotespace) {
                    clean += ' ';
                    wrotespace= true;
                }
            }
            else {
                clean += c;
                wrotespace= false;
            }
        }
        // remove trailing LWS
        if (wrotespace)
            clean.resize(clean.size()-1);
        return clean;
    }
    // replace [CR]LF  in text with CRLF<SP>
    static std::string lineencode(const std::string& str)
    {
        std::string enc; enc.reserve(str.size()*4/3);
        bool seencr= false;
        for (auto i= str.begin() ; i!=str.end() ; ++i)
        {
            char c= *i;
            if (c=='\r') {
                if (seencr) {
                    // \r\r -> output extra line
                    enc += "\r\n ";
                }
                seencr= true;
            }
            else if (c=='\n') {
                enc += "\r\n ";
                seencr= false;
            }
            else {
                enc += c;
                seencr= false;
            }
        }
        return enc;
    }
private:
    sslist _l;
public:
    HttpHeaders()
    {
    }
    // initialize from a list of kv pairs
    HttpHeaders(sslist &&L)
        : _l(std::move(L))
    {
    }
    HttpHeaders(const sslist &L)
        : _l(L)
    {
    }

    // get combined comma separated value
    // note: does not work with Set-Cookie
    std::string get(const std::string& key)
    {
        std::string val;
        for (auto i= _l.begin() ; i!=_l.end() ; ++i) {
            if (stringicompare(i->key,key)==0) {
                if (!val.empty())
                    val += ',';
                val += i->val;
            }
        }
        return val;
    }

    // return nr of values for key
    size_t multiplicity(const std::string& key)
    {
        size_t n=0;
        for (auto i= _l.begin() ; i!=_l.end() ; ++i) {
             if (stringicompare(i->key,key)==0)
                 n++;
        }
        return n;
    }
    // get single value
    std::string get(const std::string& key, size_t n)
    {
        std::string val;
        for (auto i= _l.begin() ; i!=_l.end() ; ++i) {
             if (stringicompare(i->key,key)==0) {
                 if (n==0)
                     return i->val;
                 n--;
             }
        }
        return std::string();
    }
    // adds value ( if key already exists, adds, does not replace )
    void add(const std::string& key, const std::string& val)
    {
        _l.push_back(keyval(key, val));
    }

    // sets value ( if key already exists, replaces )
    void set(const std::string& key, const std::string& val)
    {
        auto newend= std::remove_if(_l.begin(), _l.end(), [&key](const keyval& kv) {
            return stringicompare(kv.key, key)==0;
        });
        _l.erase(newend, _l.end());
        _l.push_back(keyval(key, val));
    }

    // todo: implement an 'header streamer' which 
    std::string asstring() const
    {
        std::string s;
        for (auto i= _l.begin() ; i!=_l.end() ; ++i)
        {
            s += i->key;
            s += ": ";
            s += lineencode(i->val);
            s += "\r\n";
        }
        return s;
    }
};


