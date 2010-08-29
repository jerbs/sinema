//
// Message dispatching
//
// Copyright (C) Joachim Erbs, 2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef PLATFORM_DISPATCH_HPP
#define PLATFORM_DISPATCH_HPP

template<typename TYP, template <TYP> class IF, typename OBJ, int n, bool>
struct SWITCH
{
    inline static void process(OBJ& obj, int msg_type)
    {
	const TYP msg_type_val = static_cast<TYP>(n);
	enum {last = IF<msg_type_val>::last};

	if (IF<msg_type_val>::defined == true)
	{
	    if (n == msg_type)
	    {
		typedef typename IF<msg_type_val>::type MSG_TYPE;
		// start_read_body depends on a template parameter. Unless told, the 
		// compiler cannot know that start_read_body is a template itself. 
		obj.template start_read_body<MSG_TYPE>();
		return;
	    }
	}

	// Linear search:
	SWITCH<TYP, IF, OBJ, n+1, last>::process(obj, msg_type);
    }
};

template<typename TYP, template <TYP> class IF, typename OBJ, int n>
struct SWITCH<TYP, IF, OBJ, n, true>
{
    inline static void process(OBJ&, int /* msg_type */) {}
};

template<typename TYP, template <TYP> class IF, typename OBJ>
void PROCESS(OBJ& obj, int msg_type)
{
    // Linear search:
    SWITCH<TYP, IF, OBJ, 0, false>::process(obj, msg_type);
}

#endif
