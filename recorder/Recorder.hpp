//
// Recorder
//
// Copyright (C) Joachim Erbs
//

#ifndef RECORDER_HPP
#define RECORDER_HPP

#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"
#include "recorder/GeneralEvents.hpp"

#include <boost/shared_ptr.hpp>

class Recorder : public event_receiver<Recorder>
{
    friend class event_processor<>;

public:
    Recorder(event_processor_ptr_type evt_proc)
        : base_type(evt_proc),
          mediaRecorder(0),
          fd(0)
    {}

    ~Recorder()
    {}

private:
    void process(boost::shared_ptr<RecorderInitEvent> event);

    void process(boost::shared_ptr<StartRecordingReq> event);
    void process(boost::shared_ptr<StopRecordingReq> event);

    MediaRecorder* mediaRecorder;
    boost::shared_ptr<RecorderAdapter> recorderAdapter;

    int fd;
};

#endif
