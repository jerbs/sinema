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

#include <boost/asio/io_service.hpp>

template<class Receiver>
class tcp_server
    : public boost::enable_shared_from_this<tcp_server<Receiver> >
{
    friend class tcp_acceptor;

    typedef Receiver receiver_type;
    typedef typename Receiver::tcp_connection_type tcp_connection_type;
    typedef boost::function<boost::shared_ptr<Receiver> ()> create_receiver_fct_type;

    tcp_server(boost::asio::io_service& io_service,
	       boost::asio::ip::tcp::endpoint& endpoint,
	       create_receiver_fct_type create_receiver)
	: m_acceptor(io_service, endpoint),
	  m_create_receiver(create_receiver)
    {}

    void start_accept()
    {
	TRACE_DEBUG(<< "tid = " << gettid());

	boost::shared_ptr<Receiver> receiver = m_create_receiver();

	//boost::shared_ptr<InitEvent> initEvent(new InitEvent());
	//server->queue_event(initEvent);

	boost::shared_ptr<tcp_connection_type> 
	    new_connection(new tcp_connection_type(m_acceptor.get_io_service(),
						   receiver));

	m_acceptor.async_accept(new_connection->socket(),
				boost::bind(&tcp_server::handle_accept,
					    this->shared_from_this(),
					    new_connection,
					    boost::asio::placeholders::error));
    }

    void handle_accept(boost::shared_ptr<tcp_connection_type> new_connection,
		       const boost::system::error_code& err)
    {
	TRACE_DEBUG(<< "tid = " << gettid());

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

    boost::asio::ip::tcp::acceptor m_acceptor;
    create_receiver_fct_type m_create_receiver;
};

#endif
