//
// Inter Thread Communication - Event Processor
//
// Copyright (C) Joachim Erbs, 2009-2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef EVENT_PROCESSOR_HPP
#define EVENT_PROCESSOR_HPP

#include "platform/concurrent_queue.hpp"
#include "platform/timer.hpp"
#include "platform/Logging.hpp"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <memory>

struct QuitEvent
{
};

template<class Event, class EventReceiver>
void unique_dispatch(void (EventReceiver::*process_fct)(std::unique_ptr<Event>),
		     EventReceiver* obj,
		     Event* data_ptr)
{
    std::unique_ptr<Event> event(data_ptr);
    ((*obj).*(process_fct))(std::move(event));
}

typedef boost::function<void ()> receive_fct_t;

template<class concurrent_queue = concurrent_queue<receive_fct_t> >
class event_processor
{
    friend class timer;

    typedef boost::function<void ()> receive_fct_t;
    typedef boost::function<void ()> timeout_fct_t;
    typedef boost::function<void ()> main_loop_fct_t;
    typedef concurrent_queue events_queue_t;

    events_queue_t m_events_queue;
    events_queue_t m_deferred_events_queue;
    bool m_quit;

public:
    event_processor() : m_quit(false) {}
    ~event_processor() {}

    // Simple main loop to be executed within an own thread:
    void operator()()
    {
	while(!terminating())
	{
	    dequeue_and_process();
	}
    }

    // Return callable to create Boost thread with simple main loop:
    main_loop_fct_t get_callable()
    {
	typedef void (event_processor::*op_fct_t)();
	op_fct_t tmp = &event_processor::operator();
	return boost::bind(tmp, this);
    }

    // Custom main loop provided by event_processor user:
    template<class CustomLoop>
    void operator()(CustomLoop loop)
    {
	(*loop)();
    }

    // Return callable to create Boost thread with custom main loop:
    template<class CustomLoop>
    main_loop_fct_t get_callable(CustomLoop& loop)
    {
	typedef void (event_processor::*op_fct_t)(CustomLoop& obj);
	op_fct_t tmp = &event_processor::operator();
	return boost::bind(tmp, this, loop);
    }

    // Callbacks needed to implement a custom main loop:
    void dequeue_and_process()
    {
	receive_fct_t func;
	m_events_queue.wait_and_pop(func);
	func();
    }

    // Process all queued events and return. GUI main loops are using this function:
    void dequeue_and_process_until_empty()
    {
	while(!empty())
	{
	    dequeue_and_process();
	}
    }

    // Callbacks needed to implement a custom main loop:
    bool terminating() {return m_quit;}
    bool empty() {return m_events_queue.empty();}

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

    template<class Event, class EventReceiver>
    void queue_event(std::unique_ptr<Event> event, EventReceiver* obj)
    {
	typedef void (EventReceiver::*process_fct_t)(std::unique_ptr<Event>);
	process_fct_t tmp = &EventReceiver::process;

	// Binding a unique_ptr parameter is not possible:
	Event* ptr = event.release();
	receive_fct_t fct = std::bind(unique_dispatch<Event, EventReceiver>, tmp, obj, ptr);
	
	// Here the function object is copied, this would not work,
	// if the function object contains a unique_ptr:
	m_events_queue.push(fct);
    }

    template<class Event, class EventReceiver>
    void defer_event(boost::shared_ptr<Event> event, EventReceiver* obj)
    {
	typedef void (EventReceiver::*process_fct_t)(boost::shared_ptr<Event>);
	// tmp variable avoids a static_cast<>
	process_fct_t tmp = &EventReceiver::process;
	receive_fct_t fct = boost::bind(tmp, obj, event);
	m_deferred_events_queue.push(fct);
    }

    void queue_deferred_events()
    {
	receive_fct_t fct;
	while(m_deferred_events_queue.try_pop(fct))
	{
	    m_events_queue.push(fct);
	}
    }

    template<class Event, class EventReceiver>
    void start_timer(boost::shared_ptr<Event> event, EventReceiver* obj, timer& tmr)
    {
	typedef void (EventReceiver::*process_fct_t)(boost::shared_ptr<Event>);
	// tmp variable avoids a static_cast<>
	process_fct_t tmp = &EventReceiver::process;
	receive_fct_t fct = boost::bind(tmp, obj, event);
	timeout_fct_t fct2 = boost::bind(&event_processor::timeout, this, fct);
	tmr.start_timer(fct2);
    }

    void stop_timer(timer& tmr)
    {
	tmr.stop_timer();
    }

    // This allows to also send events to the event_processor itself:
    template<class Event>
    void queue_event(boost::shared_ptr<Event> event)
    {
	queue_event(event, this);
    }

    void attach(typename concurrent_queue::notification_function_type fct)
    {
	m_events_queue.attach(fct);
    }

private:
    void process(boost::shared_ptr<QuitEvent>)
    {
	TRACE_DEBUG();
	terminate();
    }

    void timeout(receive_fct_t fct)
    {
	m_events_queue.push(fct);
    }
};

#endif
