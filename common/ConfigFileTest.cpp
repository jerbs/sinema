//
// Config File Test
//
// Copyright (C) Joachim Erbs, 2010
//

#include "common/ConfigFileTest.hpp"
#include "common/ConfigFile.hpp"

#ifdef CONFIG_FILE_TEST
#undef DEBUG
#define DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;
#endif

void ConfigFileTest::process(boost::shared_ptr<CommonInitEvent> event)
{
    configFile = event->configFile;
}

void ConfigFileTest::process(boost::shared_ptr<StartConfigFileTest> event)
{
}

void ConfigFileTest::process(boost::shared_ptr<ConfigurationData> event)
{
    DEBUG();
    configFile->queue_event(event);
}

void ConfigFileTest::process(boost::shared_ptr<ConfigurationFileWritten> event)
{
    DEBUG();
    m_event_processor->terminate();
}

ConfigFileTestApp::ConfigFileTestApp()
{
    // Create event_processor instances:
    testEventProcessor = boost::make_shared<event_processor<> >();
    applEventProcessor = boost::make_shared<event_processor<> >();

    // Create event_receiver instances:
    configFileTest = boost::make_shared<ConfigFileTest>(testEventProcessor);
    configFile = boost::make_shared<ConfigFile>(applEventProcessor);

    // Start each event_processor in an own thread.
    // Demuxer has a custom main loop:
    applThread = boost::thread( applEventProcessor->get_callable() );
}

ConfigFileTestApp::~ConfigFileTestApp()
{
    boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
    applEventProcessor->queue_event(quitEvent);
    applThread.join();
}

void ConfigFileTestApp::operator()()
{
    sendInitEvents();

    boost::shared_ptr<StartConfigFileTest> startTest(new StartConfigFileTest());
    configFileTest->queue_event(startTest);

    // Execute testEventProcessor in main thread:
    testEventProcessor->get_callable()();
}

void ConfigFileTestApp::sendInitEvents()
{
    boost::shared_ptr<CommonInitEvent> initEvent(new CommonInitEvent());
    initEvent->mediaCommon = configFileTest.get();
    initEvent->configFile = configFile;
    configFileTest->queue_event(initEvent);
    configFile->queue_event(initEvent);
}

int main(int argc, char *argv[])
{
    ConfigFileTestApp configFileTestApp;
    configFileTestApp();

    return 0;
}
