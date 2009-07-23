#ifndef VIDEO_DECODER_HPP
#define VIDEO_DECODER_HPP

#include "GeneralEvents.hpp"
#include "event_receiver.hpp"

class VideoDecoder : public event_receiver<VideoDecoder>
{
    friend class event_processor;

public:
    VideoDecoder(event_processor_ptr_type evt_proc)
	: base_type(evt_proc)
    {}
    ~VideoDecoder() {}

private:
    boost::shared_ptr<InitEvent> config;

    void process(boost::shared_ptr<InitEvent> event)
    {
	DEBUG();
	config = event;
    }

    void process(boost::shared_ptr<StartEvent> event)
    {
	DEBUG();
    }
};

#endif
