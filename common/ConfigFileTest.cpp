//
// Config File Test
//
// Copyright (C) Joachim Erbs, 2010
//
//    This file is part of Sinema.
//
//    Sinema is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Sinema is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Sinema.  If not, see <http://www.gnu.org/licenses/>.
//

#include "common/ConfigFileTest.hpp"
#include "common/ConfigFile.hpp"

#ifdef CONFIG_FILE_TEST
#undef TRACE_DEBUG
#define TRACE_DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;
#endif

void ConfigFileTest::process(boost::shared_ptr<CommonInitEvent> event)
{
    configFile = event->configFile;
}

void ConfigFileTest::process(boost::shared_ptr<ConfigurationData> event)
{
    TRACE_DEBUG();
    configFile->queue_event(event);
}

void ConfigFileTest::process(boost::shared_ptr<ConfigurationFileWritten>)
{
    TRACE_DEBUG();
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

int main()
{
    ConfigFileTestApp configFileTestApp;
    configFileTestApp();

    return 0;
}
