#ifndef DEMUXER_HPP
#define DEMUXER_HPP

#include "GeneralEvents.hpp"
#include "SystemStreamEvents.hpp"
#include "event_receiver.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

class Demuxer : public event_receiver<Demuxer>
{
    // The friend declaration allows to define the process methods private:
    friend class event_processor;

public:
    Demuxer(event_processor_ptr_type evt_proc)
	: base_type(evt_proc)
    {}
    ~Demuxer() {}

private:
    boost::shared_ptr<InitEvent> config;
    boost::shared_ptr<FileReader> fileReader;

    void process(boost::shared_ptr<InitEvent> event)
    {
	DEBUG();
	fileReader = event->fileReader;
    }

    void process(boost::shared_ptr<StartEvent> event);
    void process(boost::shared_ptr<StopEvent> event);
    void process(boost::shared_ptr<SystemStreamChunkEvent> event);
};

#endif
