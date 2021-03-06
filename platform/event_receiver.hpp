//
// Inter Thread Communication - Event Receiver
//
// Copyright (C) Joachim Erbs, 2009-2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef EVENT_RECEIVER_HPP
#define EVENT_RECEIVER_HPP

#include "platform/event_processor.hpp"

template <class MostDerived,
	  class concurrent_queue = concurrent_queue<receive_fct_t> >
class event_receiver
{
    friend class event_processor<concurrent_queue>;

    typedef MostDerived most_derived;

protected:
    typedef event_receiver<MostDerived, concurrent_queue> base_type;
    typedef boost::shared_ptr<event_processor<concurrent_queue> > event_processor_ptr_type;

private:
    event_processor_ptr_type m_event_processor;

public:
    template<class Event>
    void queue_event(boost::shared_ptr<Event> event)
    {
	m_event_processor->queue_event(event, static_cast<most_derived*>(this));
    }

    template<class Event>
    void queue_event(std::unique_ptr<Event> event)
    {
	m_event_processor->queue_event(std::move(event), static_cast<most_derived*>(this));
    }

    template<class Event>
    void defer_event(boost::shared_ptr<Event> event)
    {
	m_event_processor->defer_event(event, static_cast<most_derived*>(this));
    }

    void queue_deferred_events()
    {
	m_event_processor->queue_deferred_events();
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
    event_receiver(event_processor_ptr_type evt_proc)
	: m_event_processor(evt_proc)
    {}
    ~event_receiver() {}

    event_processor_ptr_type get_event_processor()
    {
	return m_event_processor;
    }
};

#endif
