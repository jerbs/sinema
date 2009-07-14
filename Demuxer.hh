#ifndef DEMUXER_HH
#define DEMUXER_HH

#include "ConcurrentQueue.hh"

#include <iostream>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#define DEBUG(x) std::cout << __PRETTY_FUNCTION__  x << std::endl

struct Quit
{
    Quit(){DEBUG();}
    ~Quit(){DEBUG();}
};

struct Start
{
    Start(){DEBUG();}
    ~Start(){DEBUG();}
};

struct Stop
{
    Stop(){DEBUG();}
    ~Stop(){DEBUG();}
};

class Demuxer
{
private:
    typedef boost::function<void ()> receive_fct_t;
    typedef concurrent_queue<receive_fct_t> events_queue_t;

    events_queue_t m_events_queue;

public:
    Demuxer()
	: m_quit(false)
    {}
    ~Demuxer() {}

    void operator()()
    {
	while(!m_quit)
	{
	    receive_fct_t func;
	    m_events_queue.wait_and_pop(func);
	    func();
	}
	
    }

    template<class Event>
    void queue_event(boost::shared_ptr<Event> event)
    {
	typedef void (Demuxer::*process_fct_t)(boost::shared_ptr<Event>);
	receive_fct_t fct = boost::bind(static_cast<process_fct_t>(&Demuxer::process), this, event);
	m_events_queue.push(fct);
    }

private:
    void process(boost::shared_ptr<Start> event)
    {
	DEBUG();
    }

    void process(boost::shared_ptr<Stop> event)
    {
	DEBUG();
    }

    void process(boost::shared_ptr<Quit> event)
    {
	DEBUG();
	m_quit = true;
    }

private:
    bool m_quit;
};

#endif
