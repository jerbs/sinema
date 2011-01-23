//
// gtkmm Media Player
//
// Copyright (C) Joachim Erbs, 2009-2010
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

#ifndef GTKMM_MEDIA_PLAYER_HPP
#define GTKMM_MEDIA_PLAYER_HPP

#include "player/MediaPlayer.hpp"  // indirectly include X11/Xlib.h

#include <gtkmm/drawingarea.h>
#include <glibmm/dispatcher.h>
#include <gdkmm/cursor.h>
#include <sigc++/signal.h>

class GtkmmMediaPlayer : public MediaPlayer,
			 public Gtk::DrawingArea
{
    static Glib::Dispatcher m_dispatcher;

public:
    sigc::signal<void, std::string> notificationFileName;
    sigc::signal<void, double> notificationDuration;
    sigc::signal<void, double> notificationCurrentTime;
    sigc::signal<void, NotificationCurrentVolume> notificationCurrentVolume;
    sigc::signal<void, NotificationVideoSize> notificationVideoSize;
    sigc::signal<void, NotificationClipping> notificationClipping;
    sigc::signal<void, NotificationDeinterlacerList> notificationDeinterlacerList;
    sigc::signal<void> notificationFileClosed;
    sigc::signal<void> resizeMainWindow;

    GtkmmMediaPlayer(PlayList& playList);
    virtual ~GtkmmMediaPlayer();

    static void notifyGuiThread();

    void zoom(double percent);
    void dontZoom();

    virtual void process(boost::shared_ptr<NotificationFileInfo> event);
    virtual void process(boost::shared_ptr<NotificationCurrentTime> event);
    virtual void process(boost::shared_ptr<NotificationCurrentVolume> event);
    virtual void process(boost::shared_ptr<NotificationVideoSize> event);
    virtual void process(boost::shared_ptr<NotificationClipping> event);
    virtual void process(boost::shared_ptr<NotificationDeinterlacerList> event);
    virtual void process(boost::shared_ptr<CloseFileResp> event);

    virtual bool on_main_window_state_event(GdkEventWindowState* event);

private:
    virtual void on_realize();
    virtual bool on_configure_event(GdkEventConfigure* event);
    virtual bool on_expose_event(GdkEventExpose* event);
    virtual bool on_motion_notify_event(GdkEventMotion* event);
    virtual bool on_button_press_event(GdkEventButton* event);
    virtual void on_size_allocate(Gtk::Allocation& allocation);
    virtual void process(boost::shared_ptr<HideCursorEvent> event);

    void resizeWindow();
    bool on_idle();

    template<class EVENT> void getQuadrant(EVENT* event, int&x, int&y);
    Gdk::Cursor m_cursor;
    timer m_hideCursorTimer;
    GdkCursor* m_blankGdkCursor;
    Gdk::Cursor m_blankCursor;
    bool m_fullscreen;

    // Source size of the clipped video adjusted with pixel aspect ratio:
    int m_video_width;
    int m_video_height;

    double m_video_zoom;
    bool m_zoom_enabled;
};

#endif
