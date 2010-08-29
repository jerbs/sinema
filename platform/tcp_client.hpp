//
// Inter Task Communication - tcp_client
//
// Copyright (C) Joachim Erbs, 2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef TCP_CLIENT
#define TCP_CLIENT

#include "platform/tcp_connection.hpp"

template<class Receiver>
class tcp_client
{
    typedef tcp_connection<Receiver> tcp_connection_type;

public:
    tcp_client(boost::asio::io_service& io_service,
	       boost::shared_ptr<Receiver> receiver,
	       boost::asio::ip::tcp::endpoint& endpoint)
	: m_receiver(receiver)
    {
	boost::shared_ptr<tcp_connection_type> connection(new tcp_connection_type(io_service,
										  receiver));

	connection->socket().async_connect(endpoint,
					   boost::bind(&tcp_client::handle_connect,
						       this,
						       connection,
						       boost::asio::placeholders::error));
    }

private:
    void handle_connect(boost::shared_ptr<tcp_connection_type> connection,
			const boost::system::error_code& err)
    {
	if (!err)
	{
	    connection->start_read();
	}
	else
	{
	    TRACE_ERROR(<< err.message());
	}
    }

    boost::shared_ptr<Receiver> m_receiver;
};

#endif
