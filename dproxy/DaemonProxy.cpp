//
// Daemon Proxy
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

#include "dproxy/DaemonProxy.hpp"

// ===================================================================

DaemonProxyThreadNotification::DaemonProxyThreadNotification()
{
    // Here the GUI thread is notified to call MediaReceiver::processEventQueue();
    if (m_fct)
    {
        m_fct();
    }
}

void DaemonProxyThreadNotification::setCallback(fct_t fct)
{
    m_fct = fct;
}

DaemonProxyThreadNotification::fct_t DaemonProxyThreadNotification::m_fct;

// ===================================================================

DaemonProxy::DaemonProxy()
    : base_type(boost::make_shared<event_processor<
                concurrent_queue<receive_fct_t,
                DaemonProxyThreadNotification> > >()),
      serverAutoStartEnabled(true)
{
    sysioEventProcessor = boost::make_shared<event_processor<> >();

    tcpConnector = boost::make_shared<tcp_connector>(sysioEventProcessor);
    processStarter = boost::make_shared<process_starter>(sysioEventProcessor);

    sysioThread = boost::thread( sysioEventProcessor->get_callable() );
}

DaemonProxy::~DaemonProxy()
{
    boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
    sysioEventProcessor->queue_event(quitEvent);
    sysioThread.join();
}

void DaemonProxy::init()
{
    connect();

}

// ===================================================================

void DaemonProxy::setFrequency(const ChannelData& channelData)
{
    if (proxy)
    {
	proxy->queue_event(boost::make_shared<TunerTuneChannel>(channelData));
    }
}

void DaemonProxy::startFrequencyScan(const std::string& standard)
{
    if (proxy)
    {
	proxy->queue_event(boost::make_shared<TunerStartScan>(standard));
    }
}

// ===================================================================


void DaemonProxy::processEventQueue()
{
    while(!get_event_processor()->empty())
    {
        get_event_processor()->dequeue_and_process();
    }
}

// ===================================================================

void DaemonProxy::process(boost::shared_ptr<ConnectionRefusedIndication>)
{
    TRACE_DEBUG();

    if (serverAutoStartEnabled)
    {
	boost::shared_ptr<StartProcessRequest<DaemonProxy> > req
	    (new StartProcessRequest<DaemonProxy>(this->shared_from_this(),
							"sinemad"));

	req->copyCurrentEnv();
	if (getenv("SINEMA_LOG"))
	{
	    req->insertEnv("SINEMA_LOG", "server.log");
	}

	processStarter->queue_event(req);
    }

    startRetryTimer();
}

void DaemonProxy::process(boost::shared_ptr<ConnectionTimedOut>)
{
    TRACE_DEBUG();
    startRetryTimer();
}

void DaemonProxy::startRetryTimer()
{
    timespec_t retryTime = getTimespec(5);
    retryTimer.relative(retryTime);
    start_timer(boost::make_shared<RetryTimerExpired<DaemonProxy> >
		(this->shared_from_this()),
		retryTimer);
}

void DaemonProxy::process(boost::shared_ptr<RetryTimerExpired<DaemonProxy> >)
{
    TRACE_DEBUG();
    connect();
}

void DaemonProxy::connect()
{
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), 9999);
    tcpConnector->queue_event(boost::make_shared<ConnectionRequest<DaemonProxy> >
			      (endpoint, this->shared_from_this()));
}

void DaemonProxy::process(boost::shared_ptr<ConnectionEstablished<tcp_connection_type> > event)
{
    TRACE_DEBUG( << "ConnectionEstablished");
    proxy = event->proxy;
    if (proxy)
    {
	proxy->queue_event(boost::make_shared<TunerOpen>());
    }
}

void DaemonProxy::process(boost::shared_ptr<ConnectionReleasedIndication
				<tcp_connection_type,DaemonProxy> > event)
{
    TRACE_DEBUG( << "ConnectionReleasedIndication");
    // proxy may already be reset before calling this function.
    if (proxy)
    {
	proxy.reset();
	boost::shared_ptr<tcp_connection_type> p = event->proxy;
	p->queue_event(boost::make_shared<ConnectionReleasedConfirm<tcp_connection_type> >(p));
	// Fixme: tcp_connection destructor may still be called here.
    }
}

void DaemonProxy::process(boost::shared_ptr<ConnectionReleaseResponse<DaemonProxy> >)
{
    TRACE_DEBUG( << "ConnectionReleaseResponse");
    // Nothing to do here. event may contain the 
    // last shared pointer to this object.
}

// ===================================================================

void DaemonProxy::process(boost::shared_ptr<StartProcessResponse>)
{
    TRACE_DEBUG();
}

void DaemonProxy::process(boost::shared_ptr<StartProcessFailed>)
{
    TRACE_DEBUG();
    serverAutoStartEnabled = false;
}
