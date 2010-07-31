//
// Thread-Safe Logging
//
// Copyright (C) Joachim Erbs, 2009-2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <iostream>
#include <sstream>
#include <boost/make_shared.hpp>

// -------------------------------------------------------------------

#ifndef HAVE_GETTID

#include <unistd.h>
#include <sys/syscall.h>

inline pid_t gettid() { return (pid_t) syscall (SYS_gettid); }

#endif

// -------------------------------------------------------------------

class TraceUnit : public std::stringstream
{
public:
    ~TraceUnit();
};

#define TRACE_INFO(s)          \
{                        \
   TraceUnit traceUnit;  \
   traceUnit s;          \
}

#define TRACE_DEBUG(s)                                \
{                                               \
   TraceUnit traceUnit;                         \
   traceUnit << "D: " << __PRETTY_FUNCTION__ << " " s;	\
}

#define TRACE_ERROR(s)                                            \
{                                                           \
   TraceUnit traceUnit;                                     \
   traceUnit << "Error: " << __PRETTY_FUNCTION__ << " " s;  \
   std::cout << traceUnit.str() << std::endl;	            \
}

#define TRACE_THROW(t,s)                                              \
{                                                               \
   TraceUnit traceUnit;                                         \
   traceUnit << "Exception: " << __PRETTY_FUNCTION__ << " " s;  \
   std::cout << traceUnit.str() << std::endl;			\
   throw(t(traceUnit.str()));					\
}

// -------------------------------------------------------------------
// Hex Dump
//
// Usage:
//   std::cout << hexDump(dataPtr, dataLen) << cout::endl;
//   TRACE_INFO(<< hexDump(dataPtr, dataLen) );

inline unsigned char lowerNibble(unsigned char c)
{
    return c & 0x0F;
}

inline unsigned char upperNibble(unsigned char c)
{
    return (c & 0xF0) >> 4;
}

struct _HexDump{char* data; int len;};

template<typename PTR>
inline _HexDump hexDump(PTR data, int len)
{
    _HexDump x;
    // char* pos = reinterpret_cast<char*>(data);  // static cast fails to cast from unsigned to signed
    x.data = (char*)data;
    x.len = len;
    return x;
}

template<typename _CharT, typename _Traits>
std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>& __os, _HexDump hexDump)
{
    char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    char* pos = hexDump.data;
    char* end = pos + hexDump.len;

    while(pos < end)
    {
	unsigned char c = *pos;
	__os << hex[upperNibble(c)] << hex[lowerNibble(c)];
	pos++;
    }

    return __os;
}

// -------------------------------------------------------------------

#endif
