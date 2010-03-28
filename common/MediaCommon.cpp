//
// Media Common
//
// Copyright (C) Joachim Erbs, 2010
//

#include "common/MediaCommon.hpp"
#include "common/ConfigFile.hpp"
#include "common/GeneralEvents.hpp"

#include <boost/make_shared.hpp>

// ===================================================================

MediaCommonThreadNotification::MediaCommonThreadNotification()
{
    // Here the GUI thread is notified to call MediaCommon::processEventQueue();
    if (m_fct)
    {
        m_fct();
    }
}

void MediaCommonThreadNotification::setCallback(fct_t fct)
{
    m_fct = fct;
}

MediaCommonThreadNotification::fct_t MediaCommonThreadNotification::m_fct;

// ===================================================================

MediaCommon::MediaCommon()
    : base_type(boost::make_shared<event_processor<
                concurrent_queue<receive_fct_t,
                MediaCommonThreadNotification> > >())
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

void MediaCommon::saveConfigurationData(const ConfigurationData& configurationData)
{
    // Send copy of configuration data:
    configFile->queue_event(boost::make_shared<ConfigurationData>(configurationData));
}

void MediaCommon::sendInitEvents()
{
    boost::shared_ptr<CommonInitEvent> initEvent(new CommonInitEvent());

    initEvent->mediaCommon = this;
    initEvent->configFile = configFile;

    configFile->queue_event(initEvent);
}

void MediaCommon::processEventQueue()
{
    while(!get_event_processor()->empty())
    {
        get_event_processor()->dequeue_and_process();
    }
}
