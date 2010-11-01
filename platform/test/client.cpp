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
#include "platform/process_starter.hpp"
#include "platform/tcp_client.hpp"
#include "platform/tcp_connection.hpp"
#include "platform/tcp_connector.hpp"
#include "platform/timer.hpp"

#include "ClientServerInterface.hpp"

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/bind.hpp>

#undef TRACE_DEBUG
#define TRACE_DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

struct InitEvent
{
    InitEvent(boost::shared_ptr<tcp_connector> tcpConnector,
	      boost::shared_ptr<process_starter> processStarter)
	: tcpConnector(tcpConnector),
	  processStarter(processStarter)
    {}
    boost::shared_ptr<tcp_connector> tcpConnector;
    boost::shared_ptr<process_starter> processStarter;
};

template<class Receiver>
struct RetryTimerExpired
{
    RetryTimerExpired(boost::shared_ptr<Receiver> receiver)
	: receiver(receiver)
    {}
    boost::shared_ptr<Receiver> receiver;
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
	: base_type(evt_proc),
	  eventProcessor(evt_proc),
	  serverAutoStartEnabled(true)
    {}

    ~Client()
    {}

private:
    void process(boost::shared_ptr<InitEvent> event)
    {
	TRACE_DEBUG();
	tcpConnector = event->tcpConnector;
	m_processStarter = event->processStarter;
	connect();

	listDirectory();
    }

    void process(boost::shared_ptr<ConnectionRefusedIndication>)
    {
	TRACE_DEBUG();

	if (serverAutoStartEnabled)
	{
	    boost::shared_ptr<StartProcessRequest<Client> > req
		(new StartProcessRequest<Client>(this->shared_from_this(),
						 "server"));

	    req->copyCurrentEnv();
	    if (getenv("SINEMA_LOG"))
	    {
		req->insertEnv("SINEMA_LOG", "server.log");
	    }

	    m_processStarter->queue_event(req);
	}

	startRetryTimer();
    }

    void process(boost::shared_ptr<ConnectionTimedOut>)
    {
	TRACE_DEBUG();
	startRetryTimer();
    }

    void startRetryTimer()
    {
	timespec_t retryTime = getTimespec(5);
	retryTimer.relative(retryTime);
	start_timer(boost::make_shared<RetryTimerExpired<Client> >
		    (this->shared_from_this()),
		    retryTimer);
    }

    void process(boost::shared_ptr<RetryTimerExpired<Client> >)
    {
	TRACE_DEBUG();
	connect();
    }

    void connect()
    {
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), 9999);
	tcpConnector->queue_event(boost::make_shared<ConnectionRequest<Client> >
				  (endpoint, this->shared_from_this()));
    }

    void process(boost::shared_ptr<ConnectionEstablished<tcp_connection_type> > event)
    {
	TRACE_DEBUG( << "ConnectionEstablished");
	proxy = event->proxy;

	boost::shared_ptr<csif::Indication> ind1(new csif::Indication(10,20,30));
	proxy->queue_event(ind1);

	boost::shared_ptr<csif::Indication> ind2(new csif::Indication(11,22,33));
	proxy->queue_event(ind2);

	boost::shared_ptr<csif::CreateReq> req(new csif::CreateReq(51,52,53, std::string("ping")));
	proxy->queue_event(req);
    }

    void process(boost::shared_ptr<ConnectionReleasedIndication<tcp_connection_type, Client> > event)
    {
	TRACE_DEBUG( << "ConnectionReleasedIndication");
	// proxy may already be reset before calling this function.
	proxy.reset();
	boost::shared_ptr<tcp_connection_type> p = event->proxy;
	p->queue_event(boost::make_shared<ConnectionReleasedConfirm<tcp_connection_type> >(p));
    }

    void process(boost::shared_ptr<ConnectionReleaseResponse<Client> >)
    {
	TRACE_DEBUG( << "ConnectionReleaseResponse");
	// Nothing to do here. event may contain the 
	// last shared pointer to this object.

	boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
	eventProcessor->queue_event(quitEvent);
    }

    
    void process(boost::shared_ptr<StartProcessResponse>)
    {
	TRACE_DEBUG();
    }

    void process(boost::shared_ptr<StartProcessFailed>)
    {
	TRACE_DEBUG();
	serverAutoStartEnabled = false;
    }

    void process(boost::shared_ptr<csif::CreateResp> event)
    {
	TRACE_DEBUG( << "csif::CreateResp" << *event);
	if (proxy)
	{
	    proxy->queue_event(boost::make_shared
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

    void listDirectory()
    {
	boost::shared_ptr<StartProcessRequest<Client> > req
	    (new StartProcessRequest<Client>(this->shared_from_this(),
					     "ls"));
	(*req)("-l")("-a");
	// m_processStarter->queue_event(req);
    }

    event_processor_ptr_type eventProcessor;
    boost::shared_ptr<tcp_connection_type> proxy;
    boost::shared_ptr<tcp_connector> tcpConnector;
    boost::shared_ptr<process_starter> m_processStarter;
    timer retryTimer;
    bool serverAutoStartEnabled;
};

class Appl
{
public:
    Appl()
    {
	TRACE_DEBUG(<< "tid = " << gettid());

	m_clientEventProcessor = boost::make_shared<event_processor<> >();

	m_client = boost::make_shared<Client>(m_clientEventProcessor);
	m_tcpConnector = boost::make_shared<tcp_connector>(m_clientEventProcessor);
	m_processStarter = boost::make_shared<process_starter>(m_clientEventProcessor);

	boost::shared_ptr<InitEvent> initEvent(new InitEvent(m_tcpConnector,
							     m_processStarter));
	m_client->queue_event(initEvent);
    }

    ~Appl()
    {
	boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
	m_clientEventProcessor->queue_event(quitEvent);
    }

    void run()
    {
	TRACE_DEBUG(<< "tid = " << gettid());

	(*m_clientEventProcessor)();
    }

private:
    boost::shared_ptr<event_processor<> > m_clientEventProcessor;
    boost::shared_ptr<Client> m_client;
    boost::shared_ptr<tcp_connector> m_tcpConnector;
    boost::shared_ptr<process_starter> m_processStarter;
};

int main()
{
    Appl appl;
    appl.run();

    return 0;
}
