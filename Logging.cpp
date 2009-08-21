// g++ Logging.cpp -o Logging -lboost_thread-mt

#include "Logging.hpp"
#include "event_receiver.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

class TraceReceiver : public event_receiver<TraceReceiver>
{
    friend class event_processor;
    
public:
    
    TraceReceiver(event_processor_ptr_type evt_proc)
	: base_type(evt_proc)
    {
    }

private:
    void process(boost::shared_ptr<std::string> event)
    {
	std::cout << *event << std::endl;
    }
};

class SystemTrace
{
    friend class TraceUnit;

private:
    SystemTrace()
    {
	traceEventProcessor = boost::make_shared<event_processor>();
	traceReceiver = boost::make_shared<TraceReceiver>(traceEventProcessor);
	traceThread  = boost::thread( traceEventProcessor->get_callable() );
    }

    ~SystemTrace()
    {
	boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
	traceEventProcessor->queue_event(quitEvent);
	traceThread.join();
    }

    static SystemTrace* getInstance()
    {
	if (instance == 0)
	{
	    instance = new SystemTrace();
	}

	return instance;
    }

private:
    boost::shared_ptr<event_processor> traceEventProcessor;
    boost::shared_ptr<TraceReceiver> traceReceiver;
    boost::thread traceThread;

    static SystemTrace* instance;
};

SystemTrace* SystemTrace::instance = 0;

TraceUnit::~TraceUnit()
{
    SystemTrace::getInstance()->
	traceReceiver->
	queue_event(boost::make_shared<std::string>(str()));
}
