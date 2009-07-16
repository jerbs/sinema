#ifndef AUDIO_DECODER_HPP
#define AUDIO_DECODER_HPP

#include "GeneralEvents.hpp"
#include "event_receiver.hpp"

class AudioDecoder : public event_receiver<AudioDecoder>
{
    friend class event_processor;

public:
    AudioDecoder(event_processor_ptr_type evt_proc)
	: base_type(evt_proc)
    {}
    ~AudioDecoder() {}

private:
    void process(boost::shared_ptr<Start> event)
    {
	DEBUG();
    }
};

#endif
