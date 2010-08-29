//
// Inter Task Communication - my_interface
//
// Copyright (C) Joachim Erbs, 2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MY_INTERFACE_HPP
#define MY_INTERFACE_HPP

#include <iostream>

// ===================================================================
// Message Catalog:

enum MyItfMsgTypeValues{
    MY_ITF_RESOURCE_CREATE_REQ  = 0,
    MY_ITF_RESOURCE_CREATE_RESP = 1,
    MY_ITF_INDICATION           = 2
};

// struct MyItfInvalid{};

struct MyItfResourceCreateReq
{
    MyItfResourceCreateReq() {}
    MyItfResourceCreateReq(int x, int y, int z)
	: x(x),
	  y(y),
	  z(z)
    {}
    int x;
    int y;
    int z;
};

struct MyItfResourceCreateResp
{
    MyItfResourceCreateResp() {}
    MyItfResourceCreateResp(int status)
	: status(status)
    {}
    int status;
};

struct MyItfIndication
{
    MyItfIndication() {}
    MyItfIndication(int a, int b, int c)
	: a(a), b(b), c(c)
    {}
    int a;
    int b;
    int c;
};

// Mapping needed when receiving messages:
template<MyItfMsgTypeValues MSG_TYPE_VALUE> class MyItf { public: enum {defined = false, last = false}; /* typedef MyItfInvalid type; */ };
template<> class MyItf<MY_ITF_RESOURCE_CREATE_REQ>      { public: enum {defined = true,  last = false}; typedef MyItfResourceCreateReq type; };
template<> class MyItf<MY_ITF_RESOURCE_CREATE_RESP>     { public: enum {defined = true,  last = false}; typedef MyItfResourceCreateResp type;};
template<> class MyItf<MY_ITF_INDICATION>               { public: enum {defined = true,  last = true};  typedef MyItfIndication type;};

// Mapping needed when transmitting messages:
template<typename T> inline int getMsgType() {return -1;}
template<>           inline int getMsgType<MyItfResourceCreateReq>()  {return MY_ITF_RESOURCE_CREATE_REQ;}
template<>           inline int getMsgType<MyItfResourceCreateResp>() {return MY_ITF_RESOURCE_CREATE_RESP;}
template<>           inline int getMsgType<MyItfIndication>()         {return MY_ITF_INDICATION;}


// ===================================================================
// Message Catalog additions for debugging:

template<typename T> inline const char* getMsgName(const T&){return "unknown";};
template<>           inline const char* getMsgName(const MyItfResourceCreateReq&) {return "MyItfResourceCreateReq";};
template<>           inline const char* getMsgName(const MyItfResourceCreateResp&){return "MyItfResourceCreateResp";};
template<>           inline const char* getMsgName(const MyItfIndication&)        {return "MyItfIndication";};

inline std::ostream& operator<<(std::ostream& os, const MyItfResourceCreateReq& req)
{
    return os << getMsgName(req) << "(" << req.x << "," << req.y << "," << req.z << ")";
}

inline std::ostream& operator<<(std::ostream& os, const MyItfResourceCreateResp& resp)
{
    return os << getMsgName(resp) << "(" << resp.status << ")";
}

inline std::ostream& operator<<(std::ostream& os, const MyItfIndication& ind)
{
    return os << getMsgName(ind) << "(" << ind.a << "," << ind.b << "," << ind.c << ")";
}

#endif
