//
// gtkmm Media Recorder
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
