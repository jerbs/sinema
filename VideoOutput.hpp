#ifndef VIDEO_OUTPUT_HPP
#define VIDEO_OUTPUT_HPP

#include "GeneralEvents.hpp"
#include "event_receiver.hpp"

class VideoOutput : public event_receiver<VideoOutput>
{
    friend class event_processor;

public:
    VideoOutput(event_processor_ptr_type evt_proc)
	: base_type(evt_proc)
    {}
    ~VideoOutput() {}

private:
    void process(boost::shared_ptr<Start> event)
    {
	DEBUG();
    }
};

#endif
