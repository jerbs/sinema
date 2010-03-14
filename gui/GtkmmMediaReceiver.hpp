//
// gtkmm Media Receiver
//
// Copyright (C) Joachim Erbs, 2010
//

#ifndef GTKMM_MEDIA_RECEIVER_HPP
#define GTKMM_MEDIA_RECEIVER_HPP

#include <gtkmm/drawingarea.h>
#include <glibmm/dispatcher.h>
#include <gdkmm/cursor.h>
#include <sigc++/signal.h>

#include "receiver/MediaReceiver.hpp"

struct ChannelData;

class GtkmmMediaReceiver : public MediaReceiver
{
    static Glib::Dispatcher m_dispatcher;

public:
    sigc::signal<void, const ChannelData&> notificationChannelTuned;
    sigc::signal<void, const ChannelData&> notificationSignalDetected;
    sigc::signal<void> notificationScanStopped;
    sigc::signal<void> notificationScanFinished;

    GtkmmMediaReceiver();
    virtual ~GtkmmMediaReceiver();

    static void notifyGuiThread();

private:
    virtual void process(boost::shared_ptr<TunerNotifyChannelTuned> event);
    virtual void process(boost::shared_ptr<TunerNotifySignalDetected> event);
    virtual void process(boost::shared_ptr<TunerScanStopped> event);
    virtual void process(boost::shared_ptr<TunerScanFinished> event);
};

#endif
