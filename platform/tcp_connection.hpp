//
// Inter Task Communication - tcp_connection
//
// Copyright (C) Joachim Erbs, 2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef TCP_CONNECTON_HPP
#define TCP_CONNECTON_HPP

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "platform/Logging.hpp"
#include "platform/dispatch.hpp"
#include "my_interface.hpp"        // to be removed

typedef boost::array<boost::asio::const_buffer, 2> buffer_list_type;

template<class Header, class Event>
buffer_list_type make_buffer_list(boost::shared_ptr<Header> header,
				  boost::shared_ptr<Event> event)
{
    // Using braces in the initializer list is not possible.
    // The "return value optimization" should eliminate the 
    // temporary value to hold this function's return value.
    buffer_list_type buffers = {{ boost::asio::buffer(&(*header),sizeof(Header)),
				  boost::asio::buffer(&(*event), sizeof(Event))  }};
    TRACE_DEBUG( << " length = " << sizeof(Header) << "+" << sizeof(Event));
    return buffers;
}

template<class Header, class Event>
class message
{
public:
    typedef boost::asio::const_buffer value_type;
    typedef boost::array<boost::asio::const_buffer, 2> buffer_list_type;
    typedef buffer_list_type::const_iterator const_iterator;

    message( boost::shared_ptr<Header> header,
	     boost::shared_ptr<Event> event)
	: header(header),
	  event(event),
	  buffers(make_buffer_list(header, event))
    {
	// TRACE_DEBUG();
    }

    message(const message& other)
	: header(other.header),
	  event(other.event),
	  buffers(other.buffers)
    {
	// TRACE_DEBUG();
    }

    ~message()
    {
	// TRACE_DEBUG();
    }

    const_iterator begin() const
    {
	return buffers.begin();
    }

    const_iterator end() const
    {
	return buffers.end();
    }

private:

    boost::shared_ptr<Header> header;
    boost::shared_ptr<Event> event;
    buffer_list_type buffers;
};

struct Header
{
    Header() {}
    Header(int type, int length)
	: type(type),
	  length(length)
    {}
    int type;
    int length;
};

inline std::ostream& operator<<(std::ostream& strm,
				const Header& header)
{
    strm << "(" << header.type << "," << header.length << ")";
    return strm;
}

template<class Proxy>
struct AnnounceProxy
{
    AnnounceProxy(boost::shared_ptr<Proxy> proxy)
	: proxy(proxy)
    {}
    boost::shared_ptr<Proxy> proxy;
};

template<class Receiver>
class tcp_connection
    : public boost::enable_shared_from_this<tcp_connection<Receiver> >
{
public:
    tcp_connection(boost::asio::io_service& io_service,
		   boost::shared_ptr<Receiver> receiver)
	: m_socket(io_service),
	  m_receiver(receiver)
    {}

    boost::asio::ip::tcp::socket& socket()
    {
	return m_socket;
    }

    void start_read()
    {
	TRACE_DEBUG();
	m_receiver->queue_event(boost::make_shared
				<AnnounceProxy<tcp_connection<Receiver> > >
				(this->shared_from_this()));
	start_read_header();
    }

    void start_read_header()
    {
	TRACE_DEBUG();
	boost::asio::async_read(m_socket,
				boost::asio::buffer(&m_rx_header, sizeof(m_rx_header)),
				boost::bind(&tcp_connection::handle_read_header,
					    this->shared_from_this(),
					    boost::asio::placeholders::error));
    }

    void handle_read_header(const boost::system::error_code& err)
    {
	TRACE_DEBUG();
	if (!err)
	{
	    TRACE_DEBUG( << "Header = "<< m_rx_header);
	    // start_read_body<Indication>();
	    PROCESS<MyItfMsgTypeValues, MyItf>(*this, m_rx_header.type);
	}
	else if (err != boost::asio::error::eof)
	{
	    TRACE_ERROR(<< err.message());
	}
    }

    template<class Event>
    void start_read_body()
    {
	TRACE_DEBUG();
	TRACE_DEBUG(<< "sizeof(Event) = " << sizeof(Event));
	boost::shared_ptr<Event> event(new Event());
	boost::asio::async_read(m_socket,
				boost::asio::buffer(&(*event), sizeof(Event)),
				boost::bind(&tcp_connection::handle_read_body<Event>,
					    this->shared_from_this(),
					    boost::asio::placeholders::error,
					    event));
    }

    template<class Event>
    void handle_read_body(const boost::system::error_code& err,
			  boost::shared_ptr<Event> event)
    {
	TRACE_DEBUG();
	if (!err)
	{
	    TRACE_DEBUG("ind = " << *event);
	    m_receiver->queue_event(event);
	    start_read_header();
	}
	else if (err != boost::asio::error::eof)
	{
	    TRACE_ERROR(<< err.message());
	}
    }

    template<class Event>
    void write_event(boost::shared_ptr<Event> event)
    {
	TRACE_DEBUG();
	if (m_socket.is_open())
	{
	    boost::shared_ptr<Header> header(new Header(getMsgType<Event>(), sizeof(Event)));
	    message<Header, Event> msg(header, event);
	    boost::asio::async_write(m_socket, msg,
				     boost::bind(&tcp_connection::handle_write,
						 this->shared_from_this(),
						 boost::asio::placeholders::error));
	}
    }

    void handle_write(const boost::system::error_code& err)
    {
	TRACE_DEBUG();
	if (!err)
	{
	}
	else if (err != boost::asio::error::eof)
	{
	    TRACE_ERROR(<< err.message());
	}
    }

private:
    boost::asio::ip::tcp::socket m_socket;
    boost::shared_ptr<Receiver> m_receiver;
    Header m_rx_header;
};

#endif
