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

#ifndef DAEMON_DAEMON_HPP
#define DAEMON_DAEMON_HPP

#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"
#include "platform/tcp_acceptor.hpp"

#include <boost/shared_ptr.hpp>

class Server;
class TunerFacade;

class Daemon
{
public:
    Daemon();
    ~Daemon();

    void run();

private:
    boost::shared_ptr<Server> createServer();

    boost::thread m_serverThread;
    boost::shared_ptr<event_processor<> > m_acceptorEventProcessor;
    boost::shared_ptr<event_processor<> > m_serverEventProcessor;
    boost::shared_ptr<tcp_acceptor> m_acceptor;
};

#endif
