//
// Thread-Safe Logging
//
// Copyright (C) Joachim Erbs, 2009-2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#include "Logging.hpp"
#include "event_receiver.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <fstream>
#include <stdlib.h>

class TraceReceiver : public event_receiver<TraceReceiver>
{
    friend class event_processor<>;
    
public:
    
    TraceReceiver(event_processor_ptr_type evt_proc)
	: base_type(evt_proc),
	  log()
    {
	const char* logFileName = getenv("SINEMA_LOG");
	if (logFileName)
	{
	    log.open(logFileName);
	}
    }

    inline bool isOpen()
    {
	return log.is_open();
    }

private:
    void process(boost::shared_ptr<std::string> event)
    {
	if (log.is_open())
	{
	    log << *event << std::endl;
	}
    }

    std::ofstream log;
};

class SystemTrace
{
    friend class TraceUnit;

private:
    SystemTrace()
    {
	traceEventProcessor = boost::make_shared<event_processor<> >();
	traceReceiver = boost::make_shared<TraceReceiver>(traceEventProcessor);
	traceThread  = boost::thread( traceEventProcessor->get_callable() );
    }

    ~SystemTrace()
    {
	boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
	traceEventProcessor->queue_event(quitEvent);
	traceThread.join();
    }

    static inline SystemTrace* getInstance()
    {
	if (instance == 0)
	{
	    instance = new SystemTrace();
	}

	return instance;
    }

    inline bool isEnabled()
    {
	return traceReceiver->isOpen();
    }

private:
    boost::shared_ptr<event_processor<> > traceEventProcessor;
    boost::shared_ptr<TraceReceiver> traceReceiver;
    boost::thread traceThread;

    static SystemTrace* instance;
};

SystemTrace* SystemTrace::instance = 0;

TraceUnit::~TraceUnit()
{
    SystemTrace* st = SystemTrace::getInstance();
    if (st->isEnabled())
    {
	st->traceReceiver->
	    queue_event(boost::make_shared<std::string>(str()));
    }
}
