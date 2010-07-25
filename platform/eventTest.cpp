//
// Inter Thread Communication - Event Test
//
// Copyright (C) Joachim Erbs, 2010
//

#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"

class Foo;
class Bar;

struct InitEvent
{
    boost::shared_ptr<Foo> foo;
    boost::shared_ptr<Bar> bar;
};

int fooTid = 0;
int barTid = 0;

struct Ping {
    Ping(int n)
	: n(n)
    {
	TRACE_DEBUG(<< "tid = " << gettid());
	if (gettid() != fooTid) {TRACE_ERROR(<< "Wrong thread");}
    }

    ~Ping()
    {
	TRACE_DEBUG(<< "tid = " << gettid());
	if (gettid() != barTid) {TRACE_ERROR(<< "Wrong thread");}
    }

    int n;

private:
    Ping(const Ping&);

};

struct Pong {
    Pong(int n)
	: n(n)
    {
	TRACE_DEBUG(<< "tid = " << gettid());
	if (gettid() != barTid) {TRACE_ERROR(<< "Wrong thread");}
    }

    ~Pong()
    {
	TRACE_DEBUG(<< "tid = " << gettid());
	if (gettid() != fooTid) {TRACE_ERROR(<< "Wrong thread");}
    }

    int n;
};

// -------------------------------------------------------------------

class Foo : public event_receiver<Foo>
{
    friend class event_processor<>;

public:
    Foo(event_processor_ptr_type evt_proc);
    ~Foo();

    bool terminated() {return m_terminated;}

private:
    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<Pong> event);

    boost::shared_ptr<Bar> bar;
    bool m_terminated;
    int m_count;
};

// -------------------------------------------------------------------

class Bar : public event_receiver<Bar>
{
    friend class event_processor<>;

public:
    Bar(event_processor_ptr_type evt_proc);
    ~Bar();

private:
    void process(boost::shared_ptr<InitEvent> event);
    void process(std::unique_ptr<Ping> event);

    boost::shared_ptr<Foo> foo;
};

// -------------------------------------------------------------------

class Appl
{
public:
    Appl();
    ~Appl();
    void run();

private:
    boost::thread barThread;

    boost::shared_ptr<event_processor<> > fooEventProcessor;
    boost::shared_ptr<event_processor<> > barEventProcessor;
 
    boost::shared_ptr<Foo> foo;
    boost::shared_ptr<Bar> bar;
};

// -------------------------------------------------------------------

Foo::Foo(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      m_terminated(false),
      m_count(0)
{
    TRACE_DEBUG(<< "tid = " << gettid());
}

Foo::~Foo()
{
    TRACE_DEBUG(<< "tid = " << gettid());
}

void Foo::process(boost::shared_ptr<InitEvent> event)
{
    TRACE_DEBUG(<< "tid = " << gettid());
    fooTid = gettid();
    bar = event->bar;
    std::unique_ptr<Ping> ping(new Ping(0));
    bar->queue_event(std::move(ping));
}

void Foo::process(boost::shared_ptr<Pong> event)
{
    TRACE_DEBUG(<< "tid = " << gettid() << ", n = " << event->n);
    m_count++;
    if (m_count == 10000) m_terminated = true;
    std::unique_ptr<Ping> ping(new Ping(event->n + 1));
    bar->queue_event(std::move(ping));

    // Now the ping receiver bar most probably already has deleted its
    // smart point to the ping event.
}

// -------------------------------------------------------------------

Bar::Bar(event_processor_ptr_type evt_proc)
    : base_type(evt_proc)
{
    TRACE_DEBUG(<< "tid = " << gettid());
}

Bar::~Bar()
{
    TRACE_DEBUG(<< "tid = " << gettid());
}

void Bar::process(boost::shared_ptr<InitEvent> event)
{
    TRACE_DEBUG(<< "tid = " << gettid());
    barTid = gettid();
    foo = event->foo;
}

void Bar::process(std::unique_ptr<Ping> event)
{
    TRACE_DEBUG(<< "tid = " << gettid() << ", n = " << event->n);
    foo->queue_event(boost::make_shared<Pong>(event->n + 1));
}

// -------------------------------------------------------------------

Appl::Appl()
{
    TRACE_DEBUG(<< "tid = " << gettid());

    fooEventProcessor = boost::make_shared<event_processor<> >();
    barEventProcessor = boost::make_shared<event_processor<> >();

    foo = boost::make_shared<Foo>(fooEventProcessor);
    bar = boost::make_shared<Bar>(barEventProcessor);

    barThread  = boost::thread( barEventProcessor->get_callable() );

    boost::shared_ptr<InitEvent> initEvent(new InitEvent());
    initEvent->foo = foo;
    initEvent->bar = bar;
    
    foo->queue_event(initEvent);
    bar->queue_event(initEvent);
}

Appl::~Appl()
{
    TRACE_DEBUG(<< "tid = " << gettid());

    boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
    barEventProcessor->queue_event(quitEvent);
    barThread.join();
}

void Appl::run()
{
    TRACE_DEBUG(<< "tid = " << gettid());

    while (!foo->terminated())
    {
	fooEventProcessor->dequeue_and_process();
    }
}

// -------------------------------------------------------------------

int main()
{
    std::cout << "Only Pong objects may sometimes be deleted in the wrong thread."
	      << std::endl;

    TRACE_DEBUG(<< "tid = " << gettid());

    Appl app;
    app.run();

    TRACE_DEBUG(<< "tid = " << gettid() << ", end");
}
