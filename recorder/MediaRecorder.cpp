//
// Media Recorder
//
// Copyright (C) Joachim Erbs
//

#include "recorder/MediaRecorder.hpp"
#include "recorder/PvrProtocol.hpp"
#include "recorder/GeneralEvents.hpp"
#include "recorder/Recorder.hpp"
#include "recorder/RecorderAdapter.hpp"

#include <boost/make_shared.hpp>

// ===================================================================

MediaRecorderThreadNotification::MediaRecorderThreadNotification()
{
    // Here the GUI thread is notified to call MediaRecorder::processEventQueue();
    if (m_fct)
    {
        m_fct();
    }
}

void MediaRecorderThreadNotification::setCallback(fct_t fct)
{
    m_fct = fct;
}

MediaRecorderThreadNotification::fct_t MediaRecorderThreadNotification::m_fct;

// ===================================================================

MediaRecorder::MediaRecorder()
    : base_type(boost::make_shared<event_processor<
                concurrent_queue<receive_fct_t,
                MediaRecorderThreadNotification> > >())
{
    // Create event_processor instances:
    recorderEventProcessor = boost::make_shared<event_processor<> >();

    // Create event_receiver instances:
    recorder = boost::make_shared<Recorder>(recorderEventProcessor);
    recorderAdapter = boost::make_shared<RecorderAdapter>(recorderEventProcessor);

    // Start all event_processor instance except the own one in an separate thread.
    recorderThread = boost::thread( recorderEventProcessor->get_callable() );
}

MediaRecorder::~MediaRecorder()
{
    boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
    recorderEventProcessor->queue_event(quitEvent);

    recorderThread.join();
}

void MediaRecorder::init()
{
    DEBUG();
    sendInitEvents();
    PvrProtocol::init(recorderAdapter);
    DEBUG(<< "end");
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

void MediaRecorder::processEventQueue()
{
    while(!get_event_processor()->empty())
    {
        get_event_processor()->dequeue_and_process();
    }
}
