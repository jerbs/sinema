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

#include "receiver/MediaReceiver.hpp"
#include "receiver/TunerFacade.hpp"

#include <boost/make_shared.hpp>

// ===================================================================

MediaReceiverThreadNotification::MediaReceiverThreadNotification()
{
    // Here the GUI thread is notified to call MediaReceiver::processEventQueue();
    if (m_fct)
    {
        m_fct();
    }
}

void MediaReceiverThreadNotification::setCallback(fct_t fct)
{
    m_fct = fct;
}

MediaReceiverThreadNotification::fct_t MediaReceiverThreadNotification::m_fct;

// ===================================================================

MediaReceiver::MediaReceiver()
    : base_type(boost::make_shared<event_processor<
                concurrent_queue<receive_fct_t,
                MediaReceiverThreadNotification> > >())
{
    // Create event_processor instances:
    tunerEventProcessor = boost::make_shared<event_processor<> >();

    // Create event_receiver instances:
    tunerFacade = boost::make_shared<TunerFacade>(tunerEventProcessor);

    // Start all event_processor instance except the own one in an separate thread.
    tunerThread = boost::thread( tunerEventProcessor->get_callable() );
}

MediaReceiver::~MediaReceiver()
{
    boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
    tunerEventProcessor->queue_event(quitEvent);

    tunerThread.join();
}

void MediaReceiver::init()
{
    sendInitEvents();

    tunerFacade->queue_event(boost::make_shared<TunerOpen>());
}

void MediaReceiver::setFrequency(const ChannelData& channelData)
{
    tunerFacade->queue_event(boost::make_shared<TunerTuneChannel>(channelData));
}

void MediaReceiver::startFrequencyScan(const std::string& standard)
{
    tunerFacade->queue_event(boost::make_shared<TunerStartScan>(standard));
}

void MediaReceiver::sendInitEvents()
{
    boost::shared_ptr<ReceiverInitEvent> initEvent(new ReceiverInitEvent());

    initEvent->mediaReceiver = this;
    initEvent->tunerFacade = tunerFacade;

    tunerFacade->queue_event(initEvent);
}

void MediaReceiver::processEventQueue()
{
    while(!get_event_processor()->empty())
    {
        get_event_processor()->dequeue_and_process();
    }
}

