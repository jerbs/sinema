//
// Media Recorder
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

#include "recorder/MediaRecorder.hpp"
#include "recorder/PvrProtocol.hpp"
#include "recorder/GeneralEvents.hpp"
#include "recorder/Recorder.hpp"
#include "recorder/RecorderAdapter.hpp"

#include <boost/make_shared.hpp>


MediaRecorder::MediaRecorder()
    : base_type(boost::make_shared<event_processor<
                concurrent_queue<receive_fct_t, with_callback_function> > >())
{
    // Create event_processor instances:
    recorderEventProcessor = boost::make_shared<event_processor< concurrent_queue<receive_fct_t, with_callback_function> > >();
    recorderAdapterEventProcessor = boost::make_shared<event_processor<> >();

    // Create event_receiver instances:
    recorder = boost::make_shared<Recorder>(recorderEventProcessor);
    recorderAdapter = boost::make_shared<RecorderAdapter>(recorderAdapterEventProcessor);

    // Start all event_processor instance except the own one in an separate thread.
    recorderThread = boost::thread( recorderEventProcessor->get_callable(recorder) );
    recorderAdapterThread = boost::thread( recorderAdapterEventProcessor->get_callable() );
}

MediaRecorder::~MediaRecorder()
{
    boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
    recorderEventProcessor->queue_event(quitEvent);

    recorderThread.join();
}

void MediaRecorder::init()
{
    TRACE_DEBUG();
    sendInitEvents();
    StorageProtocol::init();
    PvrProtocol::init(recorderAdapter);
}

void MediaRecorder::sendInitEvents()
{
    boost::shared_ptr<RecorderInitEvent> initEvent(new RecorderInitEvent());

    initEvent->mediaRecorder = this;
    initEvent->recorder = recorder;
    initEvent->recorderAdapter = recorderAdapter;

    recorder->queue_event(initEvent);
    recorderAdapter->queue_event(initEvent);
}
