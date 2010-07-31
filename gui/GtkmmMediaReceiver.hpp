//
// gtkmm Media Receiver
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

#ifndef GTKMM_MEDIA_RECEIVER_HPP
#define GTKMM_MEDIA_RECEIVER_HPP

#include "receiver/MediaReceiver.hpp"

#include <gtkmm/drawingarea.h>
#include <glibmm/dispatcher.h>
#include <gdkmm/cursor.h>
#include <sigc++/signal.h>

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
