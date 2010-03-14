//
// gtkmm Media Receiver
//
// Copyright (C) Joachim Erbs, 2010
//

#include "gui/GtkmmMediaReceiver.hpp"
#include "receiver/TunerFacade.hpp"

Glib::Dispatcher GtkmmMediaReceiver::m_dispatcher;

GtkmmMediaReceiver::GtkmmMediaReceiver()
    : MediaReceiver()
{
    MediaReceiverThreadNotification::setCallback(&notifyGuiThread);
    m_dispatcher.connect(sigc::mem_fun(this, &MediaReceiver::processEventQueue));
}

GtkmmMediaReceiver::~GtkmmMediaReceiver()
{
}

void GtkmmMediaReceiver::notifyGuiThread()
{
    m_dispatcher();
}

void GtkmmMediaReceiver::process(boost::shared_ptr<TunerNotifyChannelTuned> event)
{
    notificationChannelTuned(event->channelData);
}

void GtkmmMediaReceiver::process(boost::shared_ptr<TunerNotifySignalDetected> event)
{
    notificationSignalDetected(event->channelData);
}

void GtkmmMediaReceiver::process(boost::shared_ptr<TunerScanStopped> event)
{
    notificationScanStopped();
}

void GtkmmMediaReceiver::process(boost::shared_ptr<TunerScanFinished> event)
{
    notificationScanFinished();
}
