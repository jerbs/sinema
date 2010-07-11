//
// Recorder Adapter
//
// Copyright (C) Joachim Erbs
//

#include "recorder/RecorderAdapter.hpp"
#include "recorder/Recorder.hpp"

#include <boost/thread/future.hpp>
#include <sys/types.h>

void RecorderAdapter::process(boost::shared_ptr<RecorderInitEvent> event)
{
    TRACE_DEBUG(<< "tid = " << gettid());
    recorder = event->recorder;
}

void RecorderAdapter::process(boost::shared_ptr<StartRecordingSReq> event)
{
    TRACE_DEBUG();
    startSReq = event;
    recorder->queue_event(event->request);
}

void RecorderAdapter::process(boost::shared_ptr<StartRecordingResp> event)
{
    TRACE_DEBUG();
    startSReq->promise.set_value(event);
}

void RecorderAdapter::process(boost::shared_ptr<StopRecordingSReq> event)
{
    TRACE_DEBUG();
    stopSReq = event;
    recorder->queue_event(event->request);
}

void RecorderAdapter::process(boost::shared_ptr<StopRecordingResp> event)
{
    TRACE_DEBUG();
    stopSReq->promise.set_value(event);
}
