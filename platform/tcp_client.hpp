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

struct ConnectionRefusedIndication
{};

struct ConnectionTimedOut
{};

template<class Receiver>
class tcp_client
    : public boost::enable_shared_from_this<tcp_client<Receiver> >
{
    friend class tcp_connector;

    typedef typename Receiver::tcp_connection_type tcp_connection_type;

    tcp_client(boost::asio::io_service& io_service,
	       boost::shared_ptr<Receiver> receiver)
	: m_io_service(io_service),
	  m_receiver(receiver),
	  m_timer(io_service)
    {
	TRACE_DEBUG();
    }

    void connect(boost::asio::ip::tcp::endpoint& endpoint)
    {
	TRACE_DEBUG();
	boost::shared_ptr<tcp_connection_type> connection(new tcp_connection_type(m_io_service,
										  m_receiver));

	connection->socket().async_connect(endpoint,
					   boost::bind(&tcp_client::handle_connect,
						       this->shared_from_this(),
						       connection,
						       boost::asio::placeholders::error));

	m_timer.expires_from_now(boost::posix_time::seconds(1));
	m_timer.async_wait(boost::bind(&tcp_client::handle_connect_timeout,
				       this->shared_from_this(),
				       boost::asio::placeholders::error));
    }

    void handle_connect(boost::shared_ptr<tcp_connection_type> connection,
			const boost::system::error_code& err)
    {
	m_timer.cancel();

	TRACE_DEBUG(<< "err = " << err);
	if (!err)
	{
	    connection->start_read();
	}
	else if (err == boost::asio::error::connection_refused)
	{
	    m_receiver->queue_event(boost::make_shared<ConnectionRefusedIndication>());
	}
	else
	{
	    TRACE_ERROR(<< err.message());
	}
    }

    void handle_connect_timeout(const boost::system::error_code& err)
    {
	TRACE_ERROR(<< "err = " << err);
	if (err == boost::asio::error::operation_aborted)
	{
	    // Timer canceled.
	}
	else
	{
	    m_receiver->queue_event(boost::make_shared<ConnectionTimedOut>());
	}
    }

    boost::asio::io_service& m_io_service;
    boost::shared_ptr<Receiver> m_receiver;
    boost::asio::deadline_timer m_timer;
};

#endif
