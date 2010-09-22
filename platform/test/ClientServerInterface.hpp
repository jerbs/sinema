//
// Client Server Interface
//
// Copyright (C) Joachim Erbs, 2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CLIENT_SERVER_INTERFACE_HPP
#define CLIENT_SERVER_INTERFACE_HPP

#include "ClientServerEvents.hpp"
#include "platform/interface.hpp"

namespace csif
{

struct Interface
{
    typedef boost::mpl::vector<
	itf::procedure<CreateReq,   CreateResp>,
	itf::procedure<Indication,  Indication>,
	itf::procedure<DownLinkMsg, itf::none>,
	itf::procedure<itf::none,   UpLinkMsg>
    > type;
};

}

#endif
