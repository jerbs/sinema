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
