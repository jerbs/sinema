//
// gtkmm Media Recorder
//
// Copyright (C) Joachim Erbs
//

#ifndef GTKMM_MEDIA_RECORDER_HPP
#define GTKMM_MEDIA_RECORDER_HPP

#include "recorder/MediaRecorder.hpp"

#include <glibmm/dispatcher.h>
#include <sigc++/signal.h>

class GtkmmMediaRecorder : public MediaRecorder
{
    static Glib::Dispatcher m_dispatcher;

public:
    sigc::signal<void, double> notificationDuration;

    GtkmmMediaRecorder();
    virtual ~GtkmmMediaRecorder();

    static void notifyGuiThread();

    virtual void process(boost::shared_ptr<NotificationFileInfo> event);

};

#endif
