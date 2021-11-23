#pragma once
#include <string>
namespace httpparser {
    template<typename C>
    int hex2nyb(C c)
    {
        return (c>='0' && c<='9') ? c-'0'
            : (c>='A' && c<='F') ? c-'A'+10
            : (c>='a' && c<='f') ? c-'a'+10
            : -1;
    }
    inline char nyb2hex(int n)
    {
        return n<10 ? n+'0' : n+'a'-10;
    }
    template<typename I>
    std::string pctdecode(I first, I last)
    {
        int state= 0;
        uint8_t byte= 0;

        std::string str; str.reserve(last-first);
        I p= first;
        while (p!=last)
        {
            char c= *p++;
            if (state==0) {
                if (c=='%') 
                    state++;
                else if (c=='+')
                    str += ' ';
                else
                    str += c;
            }
            else if (state==1) {
                int n= hex2nyb(c);
                if (n==-1) {
                    // discard invalid chars
                    state= 0;
                }
                else {
                    byte = n<<4;
                    state++;
                }
            }
            else if (state==2) {
                int n= hex2nyb(c);
                if (n==-1) {
                    // discard invalid chars
                    state= 0;
                }
                else {
                    byte |= n;
                    str += char(byte);
                    state= 0;
                }
            }
        }
        return str;
    }
    inline std::string pctencode(const std::string& str)
    {
        std::string enc; enc.reserve(str.size()*4/3);
        for (auto i= str.begin() ; i!=str.end() ; ++i)
        {
            char c= *i;
            if (c==' ')
                enc += '+';
            else if (c<' ' || c=='%' || c=='=' || c=='&' || c=='+') {
                enc += '%';
                enc += nyb2hex((c>>4)&0xf);
                enc += nyb2hex(c&0xf);
            }
            else {
                enc += c;
            }
        }
        return enc;
    }
};
