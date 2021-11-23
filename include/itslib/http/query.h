#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "http/utils.h"
#include "stringutils.h"
class HttpQuery {
public:
    struct keyval {
        keyval(const std::string& val)
            : val(val), keyless(true)
        {}
        keyval(const  std::string& key, const std::string& val)
            : key(key), val(val), keyless(false)
        {}

        std::string key;
        std::string val;

        bool keyless;
    };
    typedef std::vector<keyval> sslist;
private:
    // note
    sslist _l;
    mutable std::string _q;

    enum { USE_L, USE_Q } _authority;

    void need_l()
    {
        if (_authority==USE_Q && _l.empty())
            parse_query();
    }
    void need_q() const
    {
        if (_authority==USE_L && _q.empty())
            make_query();
    }

    void parse_query()
    {
        // split on '&'
        // then split on '='
        // then decode '%XX' and '+' encoding

        auto p= _q.begin();
        while (p!=_q.end())
        {
            auto psep= std::find(p, _q.end(), '&');
            parsequerykv(p, psep);

            p= psep;
            if (p!=_q.end())
                ++p;
        }
    }
    template<typename I>
    void parsequerykv(I first, I last)
    {
        auto peq= std::find(first, last, '=');
        if (peq==last) {
            // keyless
            _l.push_back(keyval(httpparser::pctdecode(first, last)));
        }
        else {
            _l.push_back(keyval(httpparser::pctdecode(first, peq), httpparser::pctdecode(peq+1, last)));
        }
    }

    void make_query() const
    {
        for (auto i= _l.begin() ; i!=_l.end() ; ++i)
        {
            if (!_q.empty())
                _q += '&';
            if (i->keyless)
                _q += httpparser::pctencode(i->val);
            else {
                _q += httpparser::pctencode(i->key);
                _q += '=';
                _q += httpparser::pctencode(i->val);
            }
        }
    }

public:
    HttpQuery()
    {
    }
    // parse from querystring
    HttpQuery(const std::string& querystring)
        : _q(querystring), _authority(USE_Q)
    {
    }

    // initialize from a list of kv pairs
    HttpQuery(sslist &&L)
        : _l(std::move(L)), _authority(USE_L)
    {
    }
    HttpQuery(const sslist &L)
        : _l(L), _authority(USE_L)
    {
    }

    // get combined comma separated value
    std::string get(const std::string& key)
    {
        need_l();
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
        need_l();
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
        need_l();
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
        if (_authority==USE_Q) {
            if (!_q.empty())
                _q += '&';
            _q += httpparser::pctencode(key);
            _q += '=';
            _q += httpparser::pctencode(val);

            _l.clear();
        }
        else {  // USE_L
            _l.push_back(keyval(key, val));
            _q.clear();
        }
    }
    // add keyless value
    void add(const std::string& val)
    {
        if (_authority==USE_Q) {
            if (!_q.empty())
                _q += '&';
            _q += httpparser::pctencode(val);

            _l.clear();
        }
        else {  // USE_L
            _l.push_back(keyval(val));
            _q.clear();
        }
    }

    // sets value ( if key already exists, replaces )
    void set(const std::string& key, const std::string& val)
    {
        need_l();
        _authority=USE_L;
        auto newend= std::remove_if(_l.begin(), _l.end(), [&key](const keyval& kv)->bool {
            return stringicompare(kv.key, key)==0;
        });
        _l.erase(newend, _l.end());
        _l.push_back(keyval(key, val));
        _q.clear();
    }

    // return encoded as querystring 
    std::string querystring() const
    {
        need_q();
        return _q;
    }
    void querystring(const std::string& str)
    {
        _q= str;
        _authority= USE_Q;
        _l.clear();
    }
};


