//
// Inter Task Communication - client
//
// Copyright (C) Joachim Erbs, 2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#include "platform/Logging.hpp"

#include "platform/event_receiver.hpp"
#include "platform/tcp_client.hpp"
#include "platform/tcp_connection.hpp"

#include "ClientServerInterface.hpp"

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#undef TRACE_DEBUG
#define TRACE_DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

struct InitEvent
{
};

class Client : public event_receiver<Client>,
	       public boost::enable_shared_from_this<Client>
{
    friend class event_processor<>;

public :
    typedef tcp_connection<Client,
			   csif::Interface,
			   itf::ClientSide> tcp_connection_type;

    Client(event_processor_ptr_type evt_proc)
	: base_type(evt_proc)
    {}

    ~Client()
    {}

private:
    void process(boost::shared_ptr<InitEvent> )
    {}

    void process(boost::shared_ptr<ConnectionEstablished<tcp_connection_type> > event)
    {
	TRACE_DEBUG( << "ConnectionEstablished");
	proxy = event->proxy;

	boost::shared_ptr<csif::Indication> ind1(new csif::Indication(10,20,30));
	proxy->write_event(ind1);

	boost::shared_ptr<csif::Indication> ind2(new csif::Indication(11,22,33));
	proxy->write_event(ind2);

	boost::shared_ptr<csif::CreateReq> req(new csif::CreateReq(51,52,53, std::string("ping")));
	proxy->write_event(req);
    }

    void process(boost::shared_ptr<ConnectionReleasedIndication<tcp_connection_type, Client> > event)
    {
	TRACE_DEBUG( << "ConnectionReleasedIndication");
	// proxy may already be reset before calling this function.
	proxy.reset();
	boost::shared_ptr<tcp_connection_type> p = event->proxy;
	p->process(boost::make_shared<ConnectionReleasedConfirm<tcp_connection_type> >(p));
    }

    void process(boost::shared_ptr<ConnectionReleaseResponse<Client> >)
    {
	TRACE_DEBUG( << "ConnectionReleaseResponse");
	// Nothing to do here. event may contain the 
	// last shared pointer to this object.
    }
    
    void process(boost::shared_ptr<csif::CreateResp> event)
    {
	TRACE_DEBUG( << "csif::CreateResp" << *event);
	if (proxy)
	{
	    proxy->process(boost::make_shared
			   <ConnectionReleaseRequest<tcp_connection_type, Client> >
			   (proxy, this->shared_from_this()) );
	}
    }

    void process(boost::shared_ptr<csif::Indication> event)
    {
	TRACE_DEBUG( << "csif::Indication" << *event);
    }

    void process(boost::shared_ptr<csif::UpLinkMsg> event)
    {
	TRACE_DEBUG( << "csif::UpLinkMsg" << *event);
    }

    boost::shared_ptr<tcp_connection_type> proxy;
};

class Appl
{
public:
    Appl()
    {
	TRACE_DEBUG(<< "tid = " << gettid());

	m_clientEventProcessor = boost::make_shared<event_processor<> >();
	m_client = boost::make_shared<Client>(m_clientEventProcessor);
	m_clientThread = boost::thread( m_clientEventProcessor->get_callable() );

	boost::shared_ptr<InitEvent> initEvent(new InitEvent());
	m_client->queue_event(initEvent);
    }

    ~Appl()
    {
	boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
	m_clientEventProcessor->queue_event(quitEvent);
	m_clientThread.join();
    }

    void run()
    {
	TRACE_DEBUG(<< "tid = " << gettid());

	try
	{
	    boost::asio::io_service io_service;
	    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), 9999);
	    tcp_client<Client> tcpClient(io_service, m_client, endpoint);
	    io_service.run();
	}
	catch (std::exception& e)
	{
	    std::cerr << e.what() << std::endl;
	}
    }

private:
    boost::thread m_clientThread;
    boost::shared_ptr<event_processor<> > m_clientEventProcessor;
    boost::shared_ptr<Client> m_client;
};

int main()
{
    Appl appl;
    appl.run();

    return 0;
}
