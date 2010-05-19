//
// gtkmm Media Recorder
//
// Copyright (C) Joachim Erbs
//

#include "gui/GtkmmMediaRecorder.hpp"

Glib::Dispatcher GtkmmMediaRecorder::m_dispatcher;

GtkmmMediaRecorder::GtkmmMediaRecorder()
    : MediaRecorder()
{
    MediaRecorderThreadNotification::setCallback(&notifyGuiThread);
    m_dispatcher.connect(sigc::mem_fun(this, &MediaRecorder::processEventQueue));
}

GtkmmMediaRecorder::~GtkmmMediaRecorder()
{
}

void GtkmmMediaRecorder::notifyGuiThread()
{
    m_dispatcher();
}

void GtkmmMediaRecorder::process(boost::shared_ptr<NotificationFileInfo> event)
{
    notificationDuration(event->duration);
}
