//
// gtkmm Media Player
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef GTKMM_MEDIA_PLAYER_HPP
#define GTKMM_MEDIA_PLAYER_HPP

#include <gtkmm/drawingarea.h>
#include <glibmm/dispatcher.h>
#include <gdkmm/cursor.h>
#include <sigc++/signal.h>

#include "player/MediaPlayer.hpp"  // indirectly include X11/Xlib.h

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
    sigc::signal<void> notificationFileClosed;

    GtkmmMediaPlayer(boost::shared_ptr<PlayList> playList);
    virtual ~GtkmmMediaPlayer();

    static void notifyGuiThread();

    virtual void process(boost::shared_ptr<NotificationFileInfo> event);
    virtual void process(boost::shared_ptr<NotificationCurrentTime> event);
    virtual void process(boost::shared_ptr<NotificationCurrentVolume> event);
    virtual void process(boost::shared_ptr<NotificationVideoSize> event);
    virtual void process(boost::shared_ptr<NotificationClipping> event);
    virtual void process(boost::shared_ptr<CloseFileResp> event);

private:
    virtual void on_realize();
    virtual bool on_configure_event(GdkEventConfigure* event);
    virtual bool on_expose_event(GdkEventExpose* event);
    virtual bool on_motion_notify_event(GdkEventMotion* event);
    virtual bool on_button_press_event(GdkEventButton* event);
    virtual void process(boost::shared_ptr<HideCursorEvent> event);
    template<class EVENT> void getQuadrant(EVENT* event, int&x, int&y);
    Gdk::Cursor m_cursor;
    timer m_hideCursorTimer;
    GdkCursor* m_blankGdkCursor;
    Gdk::Cursor m_blankCursor;
};

#endif
