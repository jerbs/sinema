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
#include <sstream>
#include <string>
#include <vector>

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
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#include "platform/Logging.hpp"
#include "platform/interface.hpp"

typedef boost::array<boost::asio::const_buffer, 2> buffer_list_type;

template<int pos>
inline char get_byte(unsigned int i)
{
    return (i>>(8*pos)) & 0xff;
}

struct Header
{
    Header() {}
    Header(unsigned int type, unsigned int length)
    {
	header[0] = get_byte<1>(type);
	header[1] = get_byte<0>(type);
	header[2] = get_byte<1>(length);
	header[3] = get_byte<0>(length);
    }

    unsigned int type() const {return (header[0] << 8) | header[1];}
    unsigned int length() const {return (header[2] << 8) | header[3];}

    unsigned char header[4];
};

buffer_list_type make_buffer_list(const Header& header,
				  const std::string& body)
{
    // Using braces in the initializer list is not possible.
    // The "return value optimization" should eliminate the 
    // temporary value to hold this function's return value.
    buffer_list_type buffers = {{ boost::asio::buffer(&header,sizeof(Header)),
				  boost::asio::buffer(body)  }};
    return buffers;
}

class message
{
public:
    typedef boost::asio::const_buffer value_type;
    typedef boost::array<boost::asio::const_buffer, 2> buffer_list_type;
    typedef buffer_list_type::const_iterator const_iterator;

    message(const Header& header,
	    const std::string& body)
	: m_header(header),
	  m_body(body),
	  m_buffers(make_buffer_list(m_header, m_body))
    {
	// TRACE_DEBUG();
    }

    message(const message& other)
	: m_header(other.m_header),
	  m_body(other.m_body),
	  m_buffers(other.m_buffers)
    {
	// TRACE_DEBUG();
    }

    ~message()
    {
	// TRACE_DEBUG();
    }

    const_iterator begin() const
    {
	return m_buffers.begin();
    }

    const_iterator end() const
    {
	return m_buffers.end();
    }

private:
    Header m_header;
    std::string m_body;
    buffer_list_type m_buffers;
};

inline std::ostream& operator<<(std::ostream& strm,
				const Header& header)
{
    strm << "(" << header.type() << "," << header.length() << ")";
    return strm;
}

// -------------------------------------------------------------------

template<class Proxy>
struct ConnectionEstablished
{
    ConnectionEstablished(boost::shared_ptr<Proxy> proxy)
	: proxy(proxy)
    {}
    boost::shared_ptr<Proxy> proxy;
};

// -------------------------------------------------------------------

template<class Proxy, class Receiver>
struct ConnectionReleasedIndication
{
    ConnectionReleasedIndication(boost::shared_ptr<Proxy> proxy,
		       boost::shared_ptr<Receiver> receiver)
	: proxy(proxy),
	  receiver(receiver)
    {}
    boost::shared_ptr<Proxy> proxy;
    boost::shared_ptr<Receiver> receiver; // This delays destruction of Receiver
                                          // until this message is received.
};

template<class Proxy>
struct ConnectionReleasedConfirm
{
    ConnectionReleasedConfirm(boost::shared_ptr<Proxy> proxy)
	: proxy(proxy)
    {}
    boost::shared_ptr<Proxy> proxy;
};

template<class Proxy, class Receiver>
struct ConnectionReleaseRequest
{
    ConnectionReleaseRequest(boost::shared_ptr<Proxy> proxy,
			     boost::shared_ptr<Receiver> receiver)
	: proxy(proxy),
	  receiver(receiver)
    {}
    boost::shared_ptr<Proxy> proxy;
    boost::shared_ptr<Receiver> receiver;
};

template<class Receiver>
struct ConnectionReleaseResponse
{
    ConnectionReleaseResponse(boost::shared_ptr<Receiver> receiver)
	: receiver(receiver)
    {}
    boost::shared_ptr<Receiver> receiver; // This delays destruction of Receiver
                                          // until this message is received.
};

// -------------------------------------------------------------------

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

    typedef boost::function<void (int)> fct_t;

public:
    tcp_connection(boost::asio::io_service& io_service,
		   boost::shared_ptr<Receiver> receiver)
	: m_socket(io_service),
	  m_receiver(receiver)
    {
	TRACE_DEBUG();
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

	    typedef void (type::*member_fct_t)(int);
	    member_fct_t tmp = &type::start_read_body<typename Message::type>;
	    fct_t fct = boost::bind(tmp, parent, _1);

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
				<ConnectionEstablished<type> >
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

	    std::map<int, fct_t>::iterator pos = m_rx_map.find(m_rx_header.type());

	    if (pos != m_rx_map.end())
	    {
		// Calling start_read_body template method
		// for the received message type:
		pos->second(m_rx_header.length());
	    }
	    else
	    {
		TRACE_ERROR(<< "Invalid message type: " << m_rx_header.type());
	    }
	}
	else if (err == boost::asio::error::eof)
	{
	    handle_eof();
	}
	else if (err == boost::asio::error::operation_aborted)
	{
	    // nothing to do
	}
	else
	{
	    TRACE_ERROR(<< err.message());
	}
    }

    template<class Event>
    typename itf::enable_if_msg_in_list<Event, RxMessageList, void>::type
    start_read_body(int length)
    {
	TRACE_DEBUG();
	boost::shared_ptr<std::vector<char> > body(new std::vector<char>());
	body->resize(length);
	boost::asio::async_read(m_socket,
				boost::asio::buffer(*body),
				boost::bind(&tcp_connection::handle_read_body<Event>,
					    this->shared_from_this(),
					    boost::asio::placeholders::error,
					    body));
    }

    template<class Event>
    typename itf::enable_if_msg_in_list<Event, RxMessageList, void>::type
    handle_read_body(const boost::system::error_code& err,
		     const boost::shared_ptr<std::vector<char> > body)
    {
	TRACE_DEBUG();
	if (!err)
	{
	    // Create a stream reading from std::vector<char>:
	    std::vector<char>& b = *body;
	    boost::iostreams::stream<boost::iostreams::array_source> archive_stream(&b[0], b.size());
	    boost::archive::text_iarchive archive(archive_stream);

	    boost::shared_ptr<Event> event(new Event());
	    archive >> *event;

	    TRACE_DEBUG("ind = " << *event);
	    m_receiver->queue_event(event);
	    start_read_header();
	}
	else if (err == boost::asio::error::eof)
	{
	    handle_eof();
	}
	else if (err == boost::asio::error::operation_aborted)
	{
	    // nothing to do
	}
	else
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
	    // Serialize event:
	    std::ostringstream archive_stream;
	    boost::archive::text_oarchive archive(archive_stream);
	    archive << *event;
	    std::string body = archive_stream.str();

	    // Serialize header:
	    const int msg_id = itf::get_message_id<Event, TxMessageList>::type::value;
	    const int body_size = body.size();
	    Header header(msg_id, body_size);

	    // Asynchronously send message:
	    message msg(header, body);
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
	else if (err == boost::asio::error::eof)
	{
	    handle_eof();
	}
	else if (err == boost::asio::error::operation_aborted)
	{
	    // nothing to do
	}
	else
	{
	    TRACE_ERROR(<< err.message());
	}
    }

    void handle_eof()
    {
	m_receiver->queue_event(boost::make_shared
				<ConnectionReleasedIndication<type, Receiver> >
				(this->shared_from_this(),
				 m_receiver) );
	m_receiver.reset();
    }

    void process(boost::shared_ptr<ConnectionReleasedConfirm<type> >)
    {
	// Nothing to do here. event may contain the 
	// last shared pointer to this object.
    }

    void process(boost::shared_ptr<ConnectionReleaseRequest<type, Receiver> > event)
    {
	boost::system::error_code err;
	m_socket.close(err);

	// m_receiver may already be reset before calling this function.
	m_receiver.reset();
	boost::shared_ptr<Receiver>& receiver = event->receiver;
	receiver->queue_event(boost::make_shared
			      <ConnectionReleaseResponse<Receiver> >
			      (receiver) );
    }

private:
    boost::asio::ip::tcp::socket m_socket;
    boost::shared_ptr<Receiver> m_receiver;
    Header m_rx_header;

    std::map<int, fct_t> m_rx_map;
};

#endif
