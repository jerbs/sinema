//
// Media Common
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

#include "common/MediaCommon.hpp"
#include "common/ConfigFile.hpp"
#include "common/GeneralEvents.hpp"

#include <boost/make_shared.hpp>


MediaCommon::MediaCommon()
    : base_type(boost::make_shared<event_processor<
                concurrent_queue<receive_fct_t, with_callback_function> > >())
{
    // Create event_processor instances:
    commonEventProcessor = boost::make_shared<event_processor<> >();

    // Create event_receiver instances:
    configFile = boost::make_shared<ConfigFile>(commonEventProcessor);

    // Start all event_processor instance except the own one in an separate thread.
    commonThread = boost::thread( commonEventProcessor->get_callable() );
}

MediaCommon::~MediaCommon()
{
    boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
    commonEventProcessor->queue_event(quitEvent);

    commonThread.join();
}

void MediaCommon::init()
{
    sendInitEvents();
}

void MediaCommon::saveConfigurationData(boost::shared_ptr<ConfigurationData> event)
{
    // Send copy of configuration data:
    configFile->queue_event(boost::make_shared<ConfigurationData>(*event));
}

void MediaCommon::sendInitEvents()
{
    boost::shared_ptr<CommonInitEvent> initEvent(new CommonInitEvent());

    initEvent->mediaCommon = this;
    initEvent->configFile = configFile;

    configFile->queue_event(initEvent);
}
