//
// Recorder Adapter
//
// Copyright (C) Joachim Erbs
//

#include "recorder/RecorderAdapter.hpp"
#include "recorder/Recorder.hpp"

#include <boost/thread/future.hpp>

void RecorderAdapter::process(boost::shared_ptr<RecorderInitEvent> event)
{
    DEBUG();
    recorder = event->recorder;
}

void RecorderAdapter::process(boost::shared_ptr<StartRecordingSReq> event)
{
    DEBUG();
    startSReq = event;
    recorder->queue_event(event->request);
}

void RecorderAdapter::process(boost::shared_ptr<StartRecordingResp> event)
{
    DEBUG();
    startSReq->promise.set_value(event);
}

void RecorderAdapter::process(boost::shared_ptr<StopRecordingSReq> event)
{
    DEBUG();
    stopSReq = event;
    recorder->queue_event(event->request);
}

void RecorderAdapter::process(boost::shared_ptr<StopRecordingResp> event)
{
    DEBUG();
    stopSReq->promise.set_value(event);
}
