#ifndef EVENT_PROCESSOR_HPP
#define EVENT_PROCESSOR_HPP

#include "concurrent_queue.hpp"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

class event_processor
{
    typedef boost::function<void ()> receive_fct_t;
    typedef concurrent_queue<receive_fct_t> events_queue_t;

    events_queue_t m_events_queue;
    bool m_quit;

public:
    event_processor() : m_quit(false) {}
    ~event_processor() {}

    // Main loop to be executed within an own thread:
    void operator()()
    {
	while(!m_quit)
	{
	    receive_fct_t func;
	    m_events_queue.wait_and_pop(func);
	    func();
	}
    }

    void terminate() {m_quit = true;}

    // This interface is used by event_receiver<>:
    template<class Event, class EventReceiver>
    void queue_event(boost::shared_ptr<Event> event, EventReceiver* obj)
    {
	typedef void (EventReceiver::*process_fct_t)(boost::shared_ptr<Event>);
	// tmp variable avoids a static_cast<>
	process_fct_t tmp = &EventReceiver::process;
	receive_fct_t fct = boost::bind(tmp, obj, event);
	m_events_queue.push(fct);
    }

    // This allows to also send events to the event_processor itself:
    template<class Event>
    void queue_event(boost::shared_ptr<Event> event)
    {
	queue_event(event, this);
    }

    void process(boost::shared_ptr<Quit> event)
    {
	DEBUG();
	terminate();
    }
};

#endif
