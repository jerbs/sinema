#ifndef AUDIO_OUTPUT_HPP
#define AUDIO_OUTPUT_HPP

#include "GeneralEvents.hpp"
#include "event_receiver.hpp"

class AudioOutput : public event_receiver<AudioOutput>
{
    friend class event_processor;

public:
    AudioOutput(event_processor_ptr_type evt_proc)
	: base_type(evt_proc)
    {}
    ~AudioOutput() {}

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
