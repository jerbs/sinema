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
#include "platform/tcp_connection.hpp"
#include "platform/tcp_server.hpp"

#include "ClientServerInterface.hpp"

#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

#include <boost/mpl/for_each.hpp>

#undef TRACE_DEBUG
#define TRACE_DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

struct InitEvent
{
};

class Server : public event_receiver<Server>
{
    friend class event_processor<>;

public :
    typedef tcp_connection<Server,
			   csif::Interface,
			   itf::ServerSide> tcp_connection_type;

    Server(event_processor_ptr_type evt_proc)
	: base_type(evt_proc)
    {}

    ~Server()
    {}

private:
    void process(boost::shared_ptr<InitEvent> )
    {}

    void process(boost::shared_ptr<AnnounceProxy<tcp_connection_type> > event)
    {
	TRACE_DEBUG( << "AnnounceProxy");
	proxy = event->proxy;

	boost::shared_ptr<csif::Indication> ind(new csif::Indication(1,2,3));
	proxy->write_event(ind);
    }

    void process(boost::shared_ptr<csif::CreateReq> event)
    {
	TRACE_DEBUG( << "csif::CreateReq" << *event);
	boost::shared_ptr<csif::CreateResp> resp(new csif::CreateResp(88));
	proxy->write_event(resp);
    }

    void process(boost::shared_ptr<csif::Indication> event)
    {
	TRACE_DEBUG( << "csif::Indication" << *event);
	event->a *= 2;
	event->b *= 2;
	event->c *= 2;
	proxy->write_event(event);
    }

    void process(boost::shared_ptr<csif::DownLinkMsg> event)
    {
	TRACE_DEBUG( << "csif::DownLinkMsg" << *event);
    }

    boost::shared_ptr<tcp_connection_type> proxy;
};

class Appl
{
public:
    Appl()
    {
	TRACE_DEBUG(<< "tid = " << gettid());

	m_serverEventProcessor = boost::make_shared<event_processor<> >();
	m_server = boost::make_shared<Server>(m_serverEventProcessor);
	m_serverThread = boost::thread( m_serverEventProcessor->get_callable() );

	boost::shared_ptr<InitEvent> initEvent(new InitEvent());
	m_server->queue_event(initEvent);
    }

    ~Appl()
    {
	boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
	m_serverEventProcessor->queue_event(quitEvent);
	m_serverThread.join();
    }

    void run()
    {
	TRACE_DEBUG(<< "tid = " << gettid());

	try
	{
	    boost::asio::io_service io_service;
	    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), 9999);
	    tcp_server<Server> tcpServer(io_service, m_server, endpoint);
	    io_service.run();
	}
	catch (std::exception& e)
	{
	    std::cerr << e.what() << std::endl;
	}
    }

private:
    boost::thread m_serverThread;
    boost::shared_ptr<event_processor<> > m_serverEventProcessor;
    boost::shared_ptr<Server> m_server;
};

struct printer
{
    template<typename T>
    void operator()(T) {TRACE_DEBUG();}
};

int main()
{
    TRACE_DEBUG(<< "my_interface:");
    boost::mpl::for_each<csif::Interface::type>(printer());

    TRACE_DEBUG(<< "numbered:");
    boost::mpl::for_each<itf::numbered<csif::Interface::type>::type>(printer());

    TRACE_DEBUG(<< "downlink messages:");
    boost::mpl::for_each<itf::message_list<csif::Interface::type, itf::firstColumn>::type>(printer());

    TRACE_DEBUG(<< "uplink messages:");
    boost::mpl::for_each<itf::message_list<csif::Interface::type, itf::secondColumn>::type>(printer());

    Appl appl;
    appl.run();

    return 0;
}
