//
// Client Server Messages
//
// Copyright (C) Joachim Erbs, 2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CLIENT_SERVER_MESSAGES_HPP
#define CLIENT_SERVER_MESSAGES_HPP

namespace csif
{

struct CreateReq
{
    CreateReq() {}
    CreateReq(int x, int y, int z)
	: x(x),
	  y(y),
	  z(z)
    {}
    int x;
    int y;
    int z;
};

struct CreateResp
{
    CreateResp() {}
    CreateResp(int status)
	: status(status)
    {}
    int status;
};

struct Indication
{
    Indication() {}
    Indication(int a, int b, int c)
	: a(a), b(b), c(c)
    {}
    int a;
    int b;
    int c;
};

struct DownLinkMsg
{
};

struct UpLinkMsg
{
};

// ===================================================================
// Message Catalog additions for debugging:

template<typename T> inline const char* getMsgName(const T& msg);
#if 0
{
    return std::string("{") + typeid(msg).name() + std::string("}");
};
#else
template<>           inline const char* getMsgName(const CreateReq&)  {return "CreateReq";};
template<>           inline const char* getMsgName(const CreateResp&) {return "CreateResp";};
template<>           inline const char* getMsgName(const Indication&) {return "Indication";};
template<>           inline const char* getMsgName(const DownLinkMsg&){return "DownLinkMsg";};
template<>           inline const char* getMsgName(const UpLinkMsg&)  {return "UpLinkMsg";};
#endif

inline std::ostream& operator<<(std::ostream& os, const CreateReq& req)
{
    return os << getMsgName(req) << "(" << req.x << "," << req.y << "," << req.z << ")";
}

inline std::ostream& operator<<(std::ostream& os, const CreateResp& resp)
{
    return os << getMsgName(resp) << "(" << resp.status << ")";
}

inline std::ostream& operator<<(std::ostream& os, const Indication& ind)
{
    return os << getMsgName(ind) << "(" << ind.a << "," << ind.b << "," << ind.c << ")";
}

inline std::ostream& operator<<(std::ostream& os, const DownLinkMsg& ind)
{
    return os << getMsgName(ind);
}

inline std::ostream& operator<<(std::ostream& os, const UpLinkMsg& ind)
{
    return os << getMsgName(ind);
}

} // namespace csif

#endif
