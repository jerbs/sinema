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

#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

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
	: base_type(evt_proc),
	  num(++cnt)
    {
	TRACE_DEBUG( << "[" << num << "]");
    }

    ~Server()
    {
	TRACE_DEBUG( << "[" << num << "]");
    }

private:
    void process(boost::shared_ptr<InitEvent> )
    {}

    void process(boost::shared_ptr<ConnectionEstablished<tcp_connection_type> > event)
    {
	TRACE_DEBUG( << "[" << num << "] " << "ConnectionEstablished");
	proxy = event->proxy;

	boost::shared_ptr<csif::Indication> ind(new csif::Indication(1,2,3));
	proxy->queue_event(ind);
    }

    void process(boost::shared_ptr<ConnectionReleasedIndication<tcp_connection_type, Server> > event)
    {
	TRACE_DEBUG( << "[" << num << "] " << "ConnectionReleasedIndication");
	// proxy may already be reset before calling this function.
	proxy.reset();
	boost::shared_ptr<tcp_connection_type> p = event->proxy;
	p->queue_event(boost::make_shared<ConnectionReleasedConfirm<tcp_connection_type> >(p));
    }

#if 0
    // It not necessary to implement this method. The ConnectionReleaseResponse
    // message is sent by the template class tcp_connection. The function sending
    // the message is only generated when a ConnectionReleaseRequest is sent to
    // tcp_connection.
    void process(boost::shared_ptr<ConnectionReleaseResponse<Server> >)
    {
	TRACE_DEBUG( << "[" << num << "] " << "ConnectionReleaseResponse");
	// Nothing to do here. event may contain the 
	// last shared pointer to this object.
    }
#endif

    void process(boost::shared_ptr<csif::CreateReq> event)
    {
	TRACE_DEBUG( << "[" << num << "] " << "csif::CreateReq" << *event);
	if (proxy)
	{
	    boost::shared_ptr<csif::CreateResp> resp(new csif::CreateResp(88, std::string("pong")));
	    proxy->queue_event(resp);
	}
    }

    void process(boost::shared_ptr<csif::Indication> event)
    {
	TRACE_DEBUG( << "[" << num << "] " << "csif::Indication" << *event);
	if (proxy)
	{
	    event->a *= 2;
	    event->b *= 2;
	    event->c *= 2;
	    proxy->queue_event(event);
	}
    }

    void process(boost::shared_ptr<csif::DownLinkMsg> event)
    {
	TRACE_DEBUG( << "[" << num << "] " << "csif::DownLinkMsg" << *event);
    }

    boost::shared_ptr<tcp_connection_type> proxy;

    int num;
    static int cnt;
};

int Server::cnt = 0;

class Appl
{
public:
    Appl()
    {
	TRACE_DEBUG(<< "tid = " << gettid());

	m_serverEventProcessor = boost::make_shared<event_processor<> >();
	m_serverThread = boost::thread( m_serverEventProcessor->get_callable() );
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
	    tcp_server<Server> tcpServer(io_service,
					 boost::bind(&Appl::createServer, this),
					 endpoint);
	    io_service.run();
	}
	catch (std::exception& e)
	{
	    std::cerr << e.what() << std::endl;
	}
    }

    boost::shared_ptr<Server> createServer()
    {
	TRACE_DEBUG(<< "tid = " << gettid());

	boost::shared_ptr<Server> server = boost::make_shared<Server>(m_serverEventProcessor);

	boost::shared_ptr<InitEvent> initEvent(new InitEvent());
	server->queue_event(initEvent);

	return server;
    }

private:
    boost::thread m_serverThread;
    boost::shared_ptr<event_processor<> > m_serverEventProcessor;

};

struct printer
{
    template<typename T>
    void operator()(T) {TRACE_DEBUG();}
};

void serialize()
{
    {
	boost::shared_ptr<csif::CreateResp> resp(new csif::CreateResp(33, std::string("pong")));
	TRACE_DEBUG(<< "save: " << *resp);

	std::ofstream ofs("archive.CreateResp.msg");
        boost::archive::text_oarchive oa(ofs);
        oa << *resp;
    }

    {
	boost::shared_ptr<csif::CreateReq> req(new csif::CreateReq(51,52,53, std::string("ping")));
	TRACE_DEBUG(<< "save: " << *req);

	std::ofstream ofs("archive.CreateReq.msg");
        boost::archive::text_oarchive oa(ofs);
        oa << *req;
    }

    {
	boost::shared_ptr<csif::CreateReq> req(new csif::CreateReq());

        std::ifstream ifs("archive.CreateReq.msg");
        boost::archive::text_iarchive ia(ifs);
        ia >> *req;

        TRACE_DEBUG(<< "load: " << *req);
    }
}

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

    serialize();
    

    Appl appl;
    appl.run();

    return 0;
}
