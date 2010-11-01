//
// Server
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

#include "daemon/Server.hpp"

Server::Server(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      num(0)
{
    TRACE_DEBUG( << "[" << num << "]");

    receiverEventProcessor = boost::make_shared<event_processor<> >();
    tunerFacade = boost::make_shared<TunerFacade>(receiverEventProcessor);
    receiverThread = boost::thread( receiverEventProcessor->get_callable() );
}

Server::~Server()
{
    TRACE_DEBUG( << "[" << num << "]");

    boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
    receiverEventProcessor->queue_event(quitEvent);
    receiverThread.join();
}

void Server::process(boost::shared_ptr<InitEvent> )
{
    tunerFacade->queue_event(boost::make_shared<TunerInit>(this->shared_from_this()));
}

void Server::process(boost::shared_ptr<ConnectionEstablished<tcp_connection_type> > event)
{
    TRACE_DEBUG( << "[" << num << "] " << "ConnectionEstablished");
    proxy = event->proxy;
}

void Server::process(boost::shared_ptr<ConnectionReleasedIndication<tcp_connection_type, Server> > event)
{
    TRACE_DEBUG( << "[" << num << "] " << "ConnectionReleasedIndication");
    // proxy may already be reset before calling this function.
    if (proxy)
    {
	proxy.reset();
	boost::shared_ptr<tcp_connection_type> p = event->proxy;
	p->queue_event(boost::make_shared<ConnectionReleasedConfirm<tcp_connection_type> >(p));
	// Fixme: This does not ensure, that tcp_connection is not deleted here.
    }
}
