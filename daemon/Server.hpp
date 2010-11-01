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

#include "platform/Logging.hpp"

#include "platform/event_receiver.hpp"
#include "platform/tcp_connection.hpp"
#include "platform/tcp_server.hpp"
#include "platform/tcp_acceptor.hpp"

#include "receiver/TunerFacade.hpp"

#include "daemon/SinemadInterface.hpp"

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

struct InitEvent
{
};

class Server
    : public event_receiver<Server>,
      public boost::enable_shared_from_this<Server>
{
    friend class event_processor<>;

public :
    typedef tcp_connection<Server,
			   sdif::SinemadInterface,
			   itf::ServerSide> tcp_connection_type;

    Server(event_processor_ptr_type evt_proc);
    ~Server();

private:
    void process(boost::shared_ptr<InitEvent> );

    void process(boost::shared_ptr<ConnectionEstablished<tcp_connection_type> > event);
    void process(boost::shared_ptr<ConnectionReleasedIndication<tcp_connection_type, Server> > event);

    // Only needed when a ConnectionReleaseRequest is sent to tcp_connection:
    // void process(boost::shared_ptr<ConnectionReleaseResponse<Server> >)



    //
    void process(boost::shared_ptr<TunerOpen> event) {tunerFacade->queue_event(event);}
    void process(boost::shared_ptr<TunerClose> event) {tunerFacade->queue_event(event);}
    void process(boost::shared_ptr<TunerTuneChannel> event) {tunerFacade->queue_event(event);}
    void process(boost::shared_ptr<TunerStartScan> event) {tunerFacade->queue_event(event);}

    void process(boost::shared_ptr<TunerScanFinished> event) {if (proxy) proxy->queue_event(event);}
    void process(boost::shared_ptr<TunerScanStopped> event) {if (proxy) proxy->queue_event(event);}
    void process(boost::shared_ptr<TunerNotifySignalDetected> event) {if (proxy) proxy->queue_event(event);}
    void process(boost::shared_ptr<TunerNotifyChannelTuned> event) {if (proxy) proxy->queue_event(event);}


    boost::shared_ptr<tcp_connection_type> proxy;

    boost::thread receiverThread;
    boost::shared_ptr<event_processor<> > receiverEventProcessor;
    boost::shared_ptr<TunerFacade> tunerFacade;

    int num;
};
