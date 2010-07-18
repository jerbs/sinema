//
// Config File Test
//
// Copyright (C) Joachim Erbs, 2010
//

#ifndef CONFIG_FILE_TEST_HPP
#define CONFIG_FILE_TEST_HPP

#include "common/GeneralEvents.hpp"
#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"

#include <boost/shared_ptr.hpp>

class ConfigFile;

class ConfigFileTest : public event_receiver<ConfigFileTest>
{
    friend class event_processor<>;

public:
    ConfigFileTest(event_processor_ptr_type evt_proc)
	: base_type(evt_proc),
	  m_event_processor(evt_proc)
    {}

    ~ConfigFileTest()
    {}

private:
    boost::shared_ptr<event_processor<> > m_event_processor;
    boost::shared_ptr<ConfigFile> configFile;

    void process(boost::shared_ptr<CommonInitEvent> event);
    void process(boost::shared_ptr<ConfigurationData>);
    void process(boost::shared_ptr<ConfigurationFileWritten>);
};

class ConfigFileTestApp
{
public:
    ConfigFileTestApp();
    ~ConfigFileTestApp();

    void operator()();

private:
    // EventReceiver
    boost::shared_ptr<ConfigFileTest> configFileTest;
    boost::shared_ptr<ConfigFile> configFile;

    // Boost threads:
    boost::thread applThread;

    // EventProcessor:
    boost::shared_ptr<event_processor<> > testEventProcessor;
    boost::shared_ptr<event_processor<> > applEventProcessor;

    void sendInitEvents();
};

#endif
