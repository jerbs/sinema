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

inline std::ostream& operator<<(std::ostream& strm, const Indication& ind)
{
    strm << "(" << ind.a << "," << ind.b << "," << ind.c << ")";
    return strm;
}

#endif

