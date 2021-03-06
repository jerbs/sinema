//
// Temporary Value
//
// Copyright (C) Joachim Erbs, 2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef PLATFORM_TEMP_VALUE
#define PLATFORM_TEMP_VALUE

template <typename T>
class TemporaryValue
{
public:
    TemporaryValue(T& var_, const T& temp_val)
	: var(var_),
	  orig_val(var)
    {
	var = temp_val;
    }

    ~TemporaryValue()
    {
	var = orig_val;
    }

private:
    T& var;
    const T orig_val;
};

class TemporaryEnable : public TemporaryValue<bool>
{
public:
    typedef TemporaryValue<bool> base_type;

    TemporaryEnable(bool& var)
	: base_type(var, true)
    {}
};

class TemporaryDisable : public TemporaryValue<bool>
{
public:
    typedef TemporaryValue<bool> base_type;

    TemporaryDisable(bool& var)
	: base_type(var, false)
    {}
};

#endif
