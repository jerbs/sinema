//
// Media Receiver
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

#ifndef MEDIA_RECEIVER_HPP
#define MEDIA_RECEIVER_HPP

#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

class ChannelData;
class TunerFacade;
class TunerNotifyChannelTuned;
class TunerNotifySignalDetected;
class TunerScanStopped;
class TunerScanFinished;

class MediaReceiverThreadNotification
{
public:
    typedef void (*fct_t)();

    MediaReceiverThreadNotification();
    static void setCallback(fct_t fct);

private:
    static fct_t m_fct;
};

class MediaReceiver : public event_receiver<MediaReceiver,
					    concurrent_queue<receive_fct_t, MediaReceiverThreadNotification> >
{
    // The friend declaration allows to define the process methods private:
    friend class event_processor<concurrent_queue<receive_fct_t, MediaReceiverThreadNotification> >;
    friend class MediaReceiverThreadNotification;

public:
    MediaReceiver();
    ~MediaReceiver();

    void init();

    void setFrequency(const ChannelData& channelData);
    void startFrequencyScan(const std::string& standard);

    void processEventQueue();

protected:
    // EventReceiver
    boost::shared_ptr<TunerFacade> tunerFacade;

private:
    // Boost threads:
    boost::thread tunerThread;

    // EventProcessor:
    boost::shared_ptr<event_processor<> > tunerEventProcessor;

    virtual void process(boost::shared_ptr<TunerNotifyChannelTuned> event) = 0;
    virtual void process(boost::shared_ptr<TunerNotifySignalDetected> event) = 0;
    virtual void process(boost::shared_ptr<TunerScanStopped> event) = 0;
    virtual void process(boost::shared_ptr<TunerScanFinished> event) = 0;

    void sendInitEvents();
};

#endif
