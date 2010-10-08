//
// Inter Task Communication - tcp_connector
//
// Copyright (C) Joachim Erbs, 2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef TCP_CONNECTOR
#define TCP_CONNECTOR

#include "platform/event_receiver.hpp"
#include "platform/tcp_client.hpp"

template<class Receiver>
struct ConnectionRequest
{
    ConnectionRequest(boost::asio::ip::tcp::endpoint endpoint,
		      boost::shared_ptr<Receiver> receiver)
	: endpoint(endpoint),
	  receiver(receiver)
    {}
    boost::asio::ip::tcp::endpoint endpoint;
    boost::shared_ptr<Receiver> receiver;
};

class tcp_connector : public event_receiver<tcp_connector>,
		      public boost::enable_shared_from_this<tcp_connector>
{
    friend class event_processor<>;

public:
    tcp_connector(event_processor_ptr_type evt_proc)
        : base_type(evt_proc)
    {}
    ~tcp_connector()
    {}

private:
    template<class Receiver>
    void process(boost::shared_ptr<ConnectionRequest<Receiver> > event)
    {
	TRACE_DEBUG();
	boost::shared_ptr<tcp_client<Receiver> > tcpClient(new tcp_client<Receiver>(io_service,
										    event->receiver));

	tcpClient->connect(event->endpoint);

	asio_thread = boost::thread(boost::bind(&tcp_connector::run,
						this->shared_from_this()));
    }

    void run()
    {
	try
	{
	    TRACE_DEBUG(<< "enter");
	    io_service.run();
	    io_service.reset();
	    TRACE_DEBUG(<< "exit");
	}
	catch (std::exception& e)
	{
	    std::cerr << e.what() << std::endl;
	}
    }

    boost::asio::io_service io_service;
    boost::thread asio_thread;
};

#endif
