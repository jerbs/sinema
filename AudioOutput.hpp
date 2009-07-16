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
    void process(boost::shared_ptr<Start> event)
    {
	DEBUG();
    }
};

#endif
