//
// Recorder Adapter
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
