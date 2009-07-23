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
