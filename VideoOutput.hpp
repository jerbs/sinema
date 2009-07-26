#ifndef VIDEO_OUTPUT_HPP
#define VIDEO_OUTPUT_HPP

#include "GeneralEvents.hpp"
#include "event_receiver.hpp"

struct ShowNextFrame {};

class VideoOutput : public event_receiver<VideoOutput>
{
    friend class event_processor;

public:
    VideoOutput(event_processor_ptr_type evt_proc)
	: base_type(evt_proc)
    {
	// Periodic timer, every 0.1 seconds:
	timespec_t period;
	period.tv_sec  = 0;       
	period.tv_nsec = 100*1000*1000;  
	frameTimer.relative(period).periodic(period);
    }
    ~VideoOutput() {}

private:
    timer frameTimer;

    boost::shared_ptr<InitEvent> config;

    void process(boost::shared_ptr<InitEvent> event)
    {
	DEBUG();
	config = event;
    }

    void process(boost::shared_ptr<StartEvent> event)
    {
	DEBUG();
	boost::shared_ptr<ShowNextFrame> showNextFrameEvent(new ShowNextFrame());
	queue_event(showNextFrameEvent);
    }

    void process(boost::shared_ptr<OpenVideoOutputReq> event)
    {
	// DEBUG();
    }

    void process(boost::shared_ptr<ShowNextFrame> event)
    {
	// DEBUG();
	// start_timer(event, frameTimer);
    }
};

#endif
