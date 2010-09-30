//
// Inter Task Communication - tcp_server
//
// Copyright (C) Joachim Erbs, 2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef TCP_SERVER
#define TCP_SERVER

#include "platform/tcp_connection.hpp"

template<class Receiver>
class tcp_server
{
    typedef typename Receiver::tcp_connection_type tcp_connection_type;

public:
    tcp_server(boost::asio::io_service& io_service,
	       boost::function<boost::shared_ptr<Receiver> ()> create_receiver,
	       boost::asio::ip::tcp::endpoint& endpoint)
	: acceptor_(io_service, endpoint),
	  m_create_receiver(create_receiver)
    {
	start_accept();
    }

private:
    void start_accept()
    {
	boost::shared_ptr<Receiver> receiver = m_create_receiver();
	boost::shared_ptr<tcp_connection_type> new_connection(new tcp_connection_type(acceptor_.io_service(),
										      receiver));

	acceptor_.async_accept(new_connection->socket(),
			       boost::bind(&tcp_server::handle_accept,
					   this,
					   new_connection,
					   boost::asio::placeholders::error));
    }

    void handle_accept(boost::shared_ptr<tcp_connection_type> new_connection,
		       const boost::system::error_code& err)
    {
	if (!err)
	{
	    new_connection->start_read();
	    start_accept();
	}
	else
	{
	    TRACE_ERROR(<< err.message());
	}
    }

    boost::asio::ip::tcp::acceptor acceptor_;
    boost::function<boost::shared_ptr<Receiver> ()> m_create_receiver;
};

#endif
