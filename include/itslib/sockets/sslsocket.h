#ifndef __SSLSOCKET_H__
#define __SSLSOCKET_H__

#include "util/logmsg.h"
#include "util/singleton.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <sockets/tcpsocket.h>

#ifndef _WIN32
#include <errno.h>
#endif
// note: SSLERROR: 0,5: connect  means that the remote side has disconnected early.
// note: SSLERROR:-1,5: connect  means: consult errno
//
// note: SSLERROR: 0,1: connect  
struct sslerror {
    static int get_errno()
    {
#ifdef _WIN32
        return GetLastError();
#else
        return errno;
#endif
    }
    sslerror(const char *msg, int err, int sslerr) : _msg(msg), _err(err), _sslerr(sslerr), _errno(get_errno()), _caerror(false)
    {
        while (unsigned long e= ERR_get_error()) {
            _e.push_back(e);
            if (ERR_GET_REASON(e)==SSL_R_TLSV1_ALERT_UNKNOWN_CA)
                _caerror= true;
        }
    }
    ~sslerror() {
        std::string buf; buf.resize(1024);
        logerror("SSLERROR: %d,%d,%d: %s\n", _err, _sslerr, _errno, _msg.c_str());
        for (DwordVector::iterator i= _e.begin() ; i!=_e.end() ; ++i)
        {
            ERR_error_string_n(*i, &buf[0], buf.size());
            logerror("SSLERROR    %s\n", buf.c_str());
        }
    }
    bool iscaerror() const { return _caerror; }

    std::string _msg;
    int _err;
    int _sslerr;
    int _errno;
    DwordVector _e;
    bool _caerror;
};

class sslcontext;

class sslsocket {
    bool _isblocking;
    SSL *_ssl;

    void initssl(SSL_CTX *ctx, int fd)
    {
        _ssl = SSL_new(ctx);
        if (_ssl==NULL)
            throw sslerror("new", 0, 0);
        if (!SSL_set_fd(_ssl,fd))
            throw sslerror("setfd", 0, 0);
    }

public:
    // note: passing SSL_CTX instead of sslcontext, to avoid circular declaration issue.
    sslsocket(SSL_CTX *ctx, int fd) : _isblocking(true), _ssl(NULL)
    {
        initssl(ctx, fd);
    }
    sslsocket(SSL_CTX *ctx, tcpsocket& tcp) : _isblocking(true), _ssl(NULL)
    {
        initssl(ctx, tcp.fd());
    }

    // we could interrogate <fd> for its blocking status,
    // but for now just asking the user to explicitly notify us
    // if <fd> is in nonblocking mode
    void setnonblocking()
    {
        _isblocking= false;
    }

    // returns false if we need to call select to wait for the
    // connect to complete.
    bool connect()
    {
        int errCode = SSL_connect(_ssl);
        if (errCode <= 0) {
            int sslerr= SSL_get_error(_ssl, errCode);
            if (errCode < 0) {
                if (!_isblocking && (sslerr==SSL_ERROR_WANT_READ || sslerr==SSL_ERROR_WANT_WRITE))
                    return false;
                throw sslerror("connect", errCode, sslerr);
            }
            // else if (errCode==0)
            throw sslerror("connectaborted", errCode, sslerr);
        }
        if (SSL_get_verify_mode(_ssl)!=SSL_VERIFY_NONE) {
            if( X509_V_OK != ( errCode = SSL_get_verify_result(_ssl) ) ) {
                int sslerr= SSL_get_error(_ssl, errCode);
                throw sslerror("connectverify", errCode, sslerr);
            }
        }
        return true;
    }
    bool accept()
    {
        int errCode = SSL_accept(_ssl);
        if (errCode <= 0) {
            int sslerr= SSL_get_error(_ssl, errCode);
            if (errCode < 0) {
                if (!_isblocking && (sslerr==SSL_ERROR_WANT_READ || sslerr==SSL_ERROR_WANT_WRITE))
                    return false;
                throw sslerror("accept", errCode, sslerr);
            }
            // else if (errCode==0)
            throw sslerror("acceptabort", errCode, sslerr);
        }
        if (SSL_get_verify_mode(_ssl)!=SSL_VERIFY_NONE) {
            if( X509_V_OK != ( errCode = SSL_get_verify_result(_ssl) ) ) {
                int sslerr= SSL_get_error(_ssl, errCode);
                throw sslerror("acceptverify", errCode, sslerr);
            }
        }
        return true;
    }
    ~sslsocket()
    {
        loginfo("%s SSL_free\n", logstamp().c_str());
        SSL_free(_ssl);
        _ssl=NULL;
    }
    void close()
    {
        loginfo("%s SSL_shutdown\n", logstamp().c_str());
        if( _ssl && ( SSL_get_shutdown(_ssl) == 0 ) )
          SSL_shutdown(_ssl);
    }

    int write(const uint8_t*data, size_t size)
    {
        int errCode= SSL_write(_ssl, data, size);

        int sslerr= SSL_get_error(_ssl, errCode);
        if (errCode < 0) {
            if (!_isblocking && (sslerr==SSL_ERROR_WANT_READ || sslerr==SSL_ERROR_WANT_WRITE))
                return -1;
            throw sslerror("write", errCode, sslerr);
        }
        if (errCode== 0)
            if (sslerr!=SSL_ERROR_ZERO_RETURN)
                throw sslerror("write0", errCode, sslerr);

        return errCode;
    }

    int write(const std::string& data)
    {
        return write((const uint8_t*)data.c_str(), data.size());
    }
    int read(uint8_t *data, size_t n)
    {
        int errCode= SSL_read(_ssl, data, n);

        int sslerr= SSL_get_error(_ssl, errCode);
        if (errCode < 0) {
            if (!_isblocking && (sslerr==SSL_ERROR_WANT_READ || sslerr==SSL_ERROR_WANT_WRITE))
                return -1;
            throw sslerror("read", errCode, sslerr);
        }
        if (errCode== 0)
            if (sslerr!=SSL_ERROR_ZERO_RETURN)
                throw sslerror("read0", errCode, sslerr);

        return errCode;
    }

    int read(std::string& data, size_t n)
    {
        data.resize(n);
        int nr= read((uint8_t*)&data[0], data.size());
        if (nr==-1)
            return -1;
        data.resize(nr);
        return nr;
    }
    int pending()
    {
        return SSL_pending(_ssl);
    }
};
typedef std::shared_ptr<sslsocket> sslsocket_ptr;

// see http://www.openssl.org/docs/crypto/threads.html
class threadsafeopenssl : public Singleton<threadsafeopenssl> {
public:
    threadsafeopenssl()
    {
        CRYPTO_set_locking_callback(static_locking_function);
    }
    ~threadsafeopenssl()
    {
        CRYPTO_set_locking_callback(NULL);
        std::unique_lock<std::mutex> lock(_locksmtx);
        for (unsigned i=0 ; i<_locks.size() ; i++) {
            delete _locks[i];
            _locks[i]= NULL;
        }
    }
    static void static_locking_function(int mode, int n, const char *file, int line)
    {
        threadsafeopenssl& tso= threadsafeopenssl::instance();
        tso.locking_function(mode, n, file, line);
    }


    void locking_function(int mode, int n, const char *file, int line)
    {
        std::unique_lock<std::mutex> lock(_locksmtx);

        if (size_t(n)>=_locks.size()) {
            size_t i= _locks.size();
            _locks.resize(n+1);

            while (i<_locks.size())
            {
                _locks[i]= new std::mutex;

                i++;
            }
        }

        lock.unlock();

        if (mode&CRYPTO_LOCK) {
            _locks[n]->lock();
        }
        else {
            _locks[n]->unlock();
        }
    }

private:
    std::mutex _locksmtx;
    std::vector<std::mutex*> _locks;
};

class ssllibrary : public Singleton<ssllibrary> {
public:
    ssllibrary()
    {
        threadsafeopenssl::instance();

        SSL_load_error_strings();
        SSL_library_init();
    }
};

class sslcontext {
    SSL_CTX *_ctx;
public:
    sslcontext(bool bVerify= true) : _ctx(NULL) {

        ssllibrary::instance();

        client(bVerify);
    }
    void client(bool bVerify= true)
    {
        free();

//      _ctx = SSL_CTX_new(SSLv2_client_method());
        _ctx = SSL_CTX_new(SSLv23_client_method());
        if (_ctx==NULL)
            throw sslerror("newctx", 0, 0);
        if (bVerify)
            SSL_CTX_set_verify( _ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 0 );
        else
            SSL_CTX_set_verify( _ctx, SSL_VERIFY_NONE, 0 );

        SSL_CTX_set_options(_ctx, SSL_OP_ALL);

        logmsg("ssl in client mode\n");
//      _ctx = SSL_CTX_new(SSLv3_client_method());
/*

openssl s_client -connect 127.0.0.1:$1  -cert ../test/CA/testclient0/cert.pem  -key ../test/CA/testclient0/key.pem

/Users/itsme-plain/projects/libs/openssl-0.9.8e/demos/bio/saccept.c
/Users/itsme-plain/projects/libs/openssl-0.9.8e/demos/bio/sconnect.c
*/
    }
    void server()
    {
        free();

        _ctx = SSL_CTX_new(SSLv23_server_method());
        if (_ctx==NULL)
            throw sslerror("newctx", 0, 0);
        SSL_CTX_set_verify( _ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 0 );

        logmsg("ssl in server mode\n");
    }
    void loadcert(const std::string& certfile, const std::string& keyfile, const std::string& carootfile)
    {
        if (!_ctx)
            client();

        loginfo("sslctx: %s + %s + %s\n", certfile.c_str(), keyfile.c_str(), carootfile.c_str());
        if (!SSL_CTX_load_verify_locations(_ctx, carootfile.c_str(), 0 ) )
            throw sslerror("loadverify", 0, 0);
        if (!SSL_CTX_use_certificate_file(_ctx,certfile.c_str(),SSL_FILETYPE_PEM))
            throw sslerror("usecert", 0, 0);
        if (!SSL_CTX_use_PrivateKey_file(_ctx,keyfile.c_str(),SSL_FILETYPE_PEM))
            throw sslerror("usekey", 0, 0);
        if (!SSL_CTX_check_private_key(_ctx))
            throw sslerror("checkcert", 0, 0);
    }
    void setCipherList(const std::string& list)
    {
        if (!_ctx)
            client();
        if (!SSL_CTX_set_cipher_list(_ctx, list.c_str()))
            throw sslerror("SSL_CTX_set_cipher_list", 0, 0);
    }
    ~sslcontext()
    {
        free();
    }

    void free()
    {
        if (_ctx)
            SSL_CTX_free(_ctx);
        _ctx= NULL;
    }
    sslsocket_ptr newsocket(int fd)
    {
        return sslsocket_ptr(new sslsocket(_ctx, fd));
    }
};
typedef std::shared_ptr<sslcontext> sslcontext_ptr;

#endif
