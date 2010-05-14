//
// Recorder Adapter
//
// Copyright (C) Joachim Erbs
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
