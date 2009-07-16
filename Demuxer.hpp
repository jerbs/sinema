#ifndef DEMUXER_HPP
#define DEMUXER_HPP

#include "GeneralEvents.hpp"
#include "event_receiver.hpp"

#include <boost/shared_ptr.hpp>

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
    void process(boost::shared_ptr<Start> event)
    {
	DEBUG();
    }

    void process(boost::shared_ptr<Stop> event)
    {
	DEBUG();
    }
};

#endif
