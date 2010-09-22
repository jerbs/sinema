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
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/count_if.hpp>
#include <boost/mpl/transform_view.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/if.hpp>

#include "platform/Logging.hpp"
#include "platform/interface.hpp"

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

template<class Receiver,
	 typename Interface,
	 typename Side>
class tcp_connection
    : public boost::enable_shared_from_this<tcp_connection<Receiver,
							   Interface,
							   Side> >
{
    typedef tcp_connection<Receiver, Interface, Side> type;

    typedef typename itf::get_message_list<Interface, Side, itf::Rx>::type RxMessageList;
    typedef typename itf::get_message_list<Interface, Side, itf::Tx>::type TxMessageList;

    typedef boost::function<void ()> fct_t;

public:
    tcp_connection(boost::asio::io_service& io_service,
		   boost::shared_ptr<Receiver> receiver)
	: m_socket(io_service),
	  m_receiver(receiver)
    {
	boost::mpl::for_each<RxMessageList>(add_rx_callback(this));
    }

private:
    struct add_rx_callback
    {
	add_rx_callback(type* parent)
	    : parent(parent)
	{}

	template<typename Message>
	void operator()(Message)
	{
	    TRACE_DEBUG();

	    typedef void (type::*member_fct_t)();
	    member_fct_t tmp = &type::start_read_body<typename Message::type>;
	    fct_t fct = boost::bind(tmp, parent);

	    typedef typename Message::msg_id msg_id;

	    parent->m_rx_map.insert(std::make_pair(msg_id::value, fct));
	}

	type* parent;
    };

public:
    boost::asio::ip::tcp::socket& socket()
    {
	return m_socket;
    }

    void start_read()
    {
	TRACE_DEBUG();
	m_receiver->queue_event(boost::make_shared
				<AnnounceProxy<type> >
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

	    std::map<int, fct_t>::iterator pos = m_rx_map.find(m_rx_header.type);

	    if (pos != m_rx_map.end())
	    {
		// Calling start_read_body template method
		// for the received message type:
		pos->second();
	    }
	    else
	    {
		TRACE_ERROR(<< "Invalid message type: " << m_rx_header.type);
	    }
	}
	else if (err != boost::asio::error::eof)
	{
	    TRACE_ERROR(<< err.message());
	}
    }

    template<class Event>
    typename itf::enable_if_msg_in_list<Event, RxMessageList, void>::type
    start_read_body()
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
    typename itf::enable_if_msg_in_list<Event, RxMessageList, void>::type
    handle_read_body(const boost::system::error_code& err,
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
    typename itf::enable_if_msg_in_list<Event, TxMessageList, void>::type
    write_event(boost::shared_ptr<Event> event)
    {
	TRACE_DEBUG();
	if (m_socket.is_open())
	{
	    const int msg_id = itf::get_message_id<Event, TxMessageList>::type::value;
	    boost::shared_ptr<Header> header(new Header(msg_id, sizeof(Event)));
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

    std::map<int, fct_t> m_rx_map;
};

#endif
