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

#ifndef RECORDER_ADAPTER_HPP
#define RECORDER_ADAPTER_HPP

#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"
#include "recorder/GeneralEvents.hpp"

#include <boost/shared_ptr.hpp>

class RecorderAdapter : public event_receiver<RecorderAdapter>
{
    friend class event_processor<>;

public:
    RecorderAdapter(event_processor_ptr_type evt_proc)
        : base_type(evt_proc)
    {}

    ~RecorderAdapter()
    {}

private:
    void process(boost::shared_ptr<RecorderInitEvent> event);
    void process(boost::shared_ptr<StartRecordingSReq> event);
    void process(boost::shared_ptr<StartRecordingResp> event);
    void process(boost::shared_ptr<StopRecordingSReq> event);
    void process(boost::shared_ptr<StopRecordingResp> event);

    boost::shared_ptr<Recorder> recorder;

    boost::shared_ptr<StartRecordingSReq> startSReq;
    boost::shared_ptr<StopRecordingSReq> stopSReq;
};

#endif
