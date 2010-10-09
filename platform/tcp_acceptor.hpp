//
// Inter Task Communication - tcp_acceptor
//
// Copyright (C) Joachim Erbs, 2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef TCP_ACCEPTOR
#define TCP_ACCEPTOR

#include "platform/event_receiver.hpp"
#include "platform/tcp_server.hpp"

template<class Receiver>
struct AcceptRequest
{
    typedef boost::function<boost::shared_ptr<Receiver> ()> create_receiver_fct_type;

    AcceptRequest(boost::asio::ip::tcp::endpoint endpoint,
		  create_receiver_fct_type create_receiver)
	: endpoint(endpoint),
	  create_receiver(create_receiver)
    {}
    typedef Receiver receiver_type;
    boost::asio::ip::tcp::endpoint endpoint;
    create_receiver_fct_type create_receiver;
};

class tcp_acceptor : public event_receiver<tcp_acceptor>,
		     public boost::enable_shared_from_this<tcp_acceptor>
{
    friend class event_processor<>;

public:
    tcp_acceptor(event_processor_ptr_type evt_proc)
        : base_type(evt_proc)
    {}
    ~tcp_acceptor()
    {}

private:
    template<class Receiver>
    void process(boost::shared_ptr<AcceptRequest<Receiver> > event)
    {
	TRACE_DEBUG();
	boost::shared_ptr<tcp_server<Receiver> >
	    tcpServer(new tcp_server<Receiver>(io_service,
					       event->endpoint,
					       event->create_receiver));

	tcpServer->start_accept();

	asio_thread = boost::thread(boost::bind(&tcp_acceptor::run,
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
