//
// Daemon
//
// Copyright (C) Joachim Erbs, 2010
//
//    This file is part of Sinema.
//
//    Sinema is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Sinema is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Sinema.  If not, see <http://www.gnu.org/licenses/>.
//

#include "platform/Logging.hpp"
#include "daemon/Daemon.hpp"
#include "daemon/Server.hpp"

Daemon::Daemon()
{
    TRACE_DEBUG(<< "tid = " << gettid());

    m_acceptorEventProcessor = boost::make_shared<event_processor<> >();
    m_acceptor = boost::make_shared<tcp_acceptor>(m_acceptorEventProcessor);

    m_serverEventProcessor = boost::make_shared<event_processor<> >();
    m_serverThread = boost::thread( m_serverEventProcessor->get_callable() );

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), 9999);
    m_acceptor->queue_event(boost::make_shared<AcceptRequest<Server> >
			    (endpoint,
			     boost::bind(&Daemon::createServer, this)));
}

Daemon::~Daemon()
{
    boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
    m_acceptorEventProcessor->queue_event(quitEvent);
    m_serverEventProcessor->queue_event(quitEvent);
    m_serverThread.join();
}

void Daemon::run()
{
    TRACE_DEBUG(<< "tid = " << gettid());

    (*m_acceptorEventProcessor)();
}

boost::shared_ptr<Server> Daemon::createServer()
{
    TRACE_DEBUG(<< "tid = " << gettid());

    boost::shared_ptr<Server> server = boost::make_shared<Server>(m_serverEventProcessor);

    boost::shared_ptr<InitEvent> initEvent(new InitEvent());
    server->queue_event(initEvent);

    return server;
}
