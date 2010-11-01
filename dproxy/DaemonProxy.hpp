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

#ifndef DAEMON_PROXY_HPP
#define DAEMON_PROXY_HPP

#include "platform/Logging.hpp"

#include "platform/event_receiver.hpp"
#include "platform/process_starter.hpp"
#include "platform/tcp_client.hpp"
#include "platform/tcp_connection.hpp"
#include "platform/tcp_connector.hpp"
#include "platform/timer.hpp"

#include "daemon/SinemadInterface.hpp"

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/bind.hpp>

class ChannelData;
class TunerNotifyChannelTuned;
class TunerNotifySignalDetected;
class TunerScanStopped;
class TunerScanFinished;

template<class Receiver>
struct RetryTimerExpired
{
    RetryTimerExpired(boost::shared_ptr<Receiver> receiver)
	: receiver(receiver)
    {}
    boost::shared_ptr<Receiver> receiver;
};

class DaemonProxyThreadNotification
{
public:
    typedef void (*fct_t)();

    DaemonProxyThreadNotification();
    static void setCallback(fct_t fct);

private:
    static fct_t m_fct;
};

class DaemonProxy : public event_receiver<DaemonProxy,
					  concurrent_queue<receive_fct_t, DaemonProxyThreadNotification> >,
			  public boost::enable_shared_from_this<DaemonProxy>
{
    friend class event_processor<concurrent_queue<receive_fct_t, DaemonProxyThreadNotification> >;
    friend class DaemonProxyThreadNotification;

public:
    typedef tcp_connection<DaemonProxy,
			   sdif::SinemadInterface,
			   itf::ClientSide> tcp_connection_type;

    DaemonProxy();
    ~DaemonProxy();

    void init();

    void setFrequency(const ChannelData& channelData);
    void startFrequencyScan(const std::string& standard);

    void processEventQueue();

private:
    // Events received from tcp_client and tcp_connection:
    void process(boost::shared_ptr<ConnectionRefusedIndication>);
    void process(boost::shared_ptr<ConnectionTimedOut>);
    void process(boost::shared_ptr<RetryTimerExpired<DaemonProxy> >);
    void process(boost::shared_ptr<ConnectionEstablished<tcp_connection_type> > event);
    void process(boost::shared_ptr<ConnectionReleasedIndication<tcp_connection_type, DaemonProxy> > event);
    void process(boost::shared_ptr<ConnectionReleaseResponse<DaemonProxy> >);
    void startRetryTimer();
    void connect();

    // Events received from TunerFacade:
    virtual void process(boost::shared_ptr<TunerNotifyChannelTuned> event) = 0;
    virtual void process(boost::shared_ptr<TunerNotifySignalDetected> event) = 0;
    virtual void process(boost::shared_ptr<TunerScanStopped> event) = 0;
    virtual void process(boost::shared_ptr<TunerScanFinished> event) = 0;

    // Events received from process_starter:
    void process(boost::shared_ptr<StartProcessResponse>);
    void process(boost::shared_ptr<StartProcessFailed>);

    boost::shared_ptr<event_processor<> > sysioEventProcessor;
    boost::thread sysioThread;
    boost::shared_ptr<tcp_connection_type> proxy;
    boost::shared_ptr<tcp_connector> tcpConnector;
    boost::shared_ptr<process_starter> processStarter;
    timer retryTimer;
    bool serverAutoStartEnabled;
};

#endif
