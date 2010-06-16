#ifndef EVENT_RECEIVER_HPP
#define EVENT_RECEIVER_HPP

#include "event_processor.hpp"

template <class MostDerived>
class event_receiver
{
    friend class event_processor;
    typedef MostDerived most_derived;

    boost::shared_ptr<event_processor> m_event_processor;

public:
    template<class Event>
    void queue_event(boost::shared_ptr<Event> event)
    {
	m_event_processor->queue_event(event, static_cast<most_derived*>(this));
    }

    template<class Event>
    void start_timer(boost::shared_ptr<Event> event, timer& t)
    {
	m_event_processor->start_timer(event, static_cast<most_derived*>(this), t);
    }

    void stop_timer(timer& t)
    {
	m_event_processor->stop_timer(t);
    }

protected:
    typedef event_receiver<MostDerived> base_type;
    typedef boost::shared_ptr<event_processor> event_processor_ptr_type;

    event_receiver(event_processor_ptr_type evt_proc)
	: m_event_processor(evt_proc)
    {}
    ~event_receiver() {}

    void terminate()
    {
	m_event_processor->terminate();
    }
};

#endif
