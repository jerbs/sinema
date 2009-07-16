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
    void process(boost::shared_ptr<Start> event)
    {
	DEBUG();
    }
};

#endif
