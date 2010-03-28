//
// gtkmm Media Common
//
// Copyright (C) Joachim Erbs, 2010
//

#include "gui/GtkmmMediaCommon.hpp"
// #include "receiver/TunerFacade.hpp"

Glib::Dispatcher GtkmmMediaCommon::m_dispatcher;

GtkmmMediaCommon::GtkmmMediaCommon()
    : MediaCommon()
{
    MediaCommonThreadNotification::setCallback(&notifyGuiThread);
    m_dispatcher.connect(sigc::mem_fun(this, &MediaCommon::processEventQueue));
}

GtkmmMediaCommon::~GtkmmMediaCommon()
{
}

void GtkmmMediaCommon::notifyGuiThread()
{
    m_dispatcher();
}

void GtkmmMediaCommon::process(boost::shared_ptr<ConfigurationData> event)
{
    signal_configuration_data_loaded(*event);
}
