#pragma once
#include <string>
#include "http/query.h"
#include "http/headers.h"

// request contains information to build the request, and it also collects the results
class HttpRequest {

    std::string _version;
    std::string _method;
    std::string _path;
    mutable std::string _fullrequest; bool _fr_valid;
    HttpQuery _query;         bool _havequery;
    HttpHeaders _headers;

    void need_fr() const
    {
        if (_fr_valid)
            return;

        _fullrequest.clear();
        _fullrequest.reserve(_path.size()+256);

        _fullrequest.append(_path);
        if (_havequery) {
            _fullrequest.append("?");
            _fullrequest.append(_query.querystring());
        }
    }
public:
    // used by server, which adds request read from client.
    HttpRequest()
    {
    }

    // sends request with query string
    // note: when query is empty, the '?' will be sent.
    HttpRequest(const std::string& method, const std::string& path, const HttpQuery& query, const HttpHeaders& hdrs)
        : _version("1.1"), _method(method), _path(path), _fr_valid(false), _query(query), _havequery(true), _headers(hdrs)
    {
    }

    // sends request without query string
    HttpRequest(const std::string& method, const std::string& path, const HttpHeaders& hdrs)
        : _version("1.1"), _method(method), _path(path), _fr_valid(false), _havequery(false), _headers(hdrs)
    {
    }


    std::string version() const { return _version; }
    void version(const std::string& v) { _version= v; }

    std::string method() const { return _method; }
    void method(const std::string& m) { _method= m; }

    // returns raw /path?query
    std::string request() const
    {
        need_fr();
        return _fullrequest;
    }
    void request(const std::string& req)
    {
        _fullrequest= req;
        _fr_valid= true;

        size_t pq= req.find('?');
        _havequery= pq!=req.npos;

        if (_havequery) {
            _path= req.substr(0, pq);
            _query.querystring(req.substr(pq+1));
        }
        else {
            _path= req;
        }
    }

    // returns /path
    std::string path() const { return _path; }

    //---------- query access --------------
    // sets query object
    void query(const HttpQuery& q)
    {
        _query= q;
        _fr_valid= false;
        _fullrequest.clear();
        _havequery= true;
    }
    // returns parsed query
    HttpQuery& query()
    {
        return _query;
    }
    // access query values:
    std::string param(const std::string& key)
    {
        return _query.get(key);
    }
    void setparam(const std::string& key, const std::string& value)
    {
        _query.set(key, value);
    }
    void addparam(const std::string& key, const std::string& value)
    {
        _query.add(key, value);
    }

    //----------- header access -------------
    // sets the header object
    void headers(const HttpHeaders&hdrs)
    {
        _headers= hdrs;
    }
    // returns parsed headers
    HttpHeaders& headers() { return _headers; }
    const HttpHeaders& headers() const { return _headers; }
    // access header values:
    std::string header(const std::string& key)
    {
        return _headers.get(key);
    }
    void setheader(const std::string& key, const std::string& value)
    {
        _headers.set(key, value);
    }
    void addheader(const std::string& key, const std::string& value)
    {
        _headers.add(key, value);
    }
};
