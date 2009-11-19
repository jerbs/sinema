//
// gtkmm Media Player
//
// Copyright (C) Joachim Erbs, 2009
//

#include "gui/GtkmmMediaPlayer.hpp"

#include <gdk/gdkx.h>

// -------------------------------------------------------------------

Display * get_x_display(const Glib::RefPtr<Gdk::Drawable> & drawable)
{
    return gdk_x11_drawable_get_xdisplay(drawable->gobj());
}

Display * get_x_display(Gtk::Widget & widget)
{
    Glib::RefPtr<Gdk::Drawable> window(widget.get_window());
    assert(window);
    return get_x_display(window);
}

Window get_x_window(const Glib::RefPtr<Gdk::Drawable> & drawable)
{
    return gdk_x11_drawable_get_xid(drawable->gobj());
}

Window get_x_window(Gtk::Widget & widget)
{
    Glib::RefPtr<Gdk::Drawable> window(widget.get_window());
    assert(window);
    return get_x_window(window);
}

// -------------------------------------------------------------------

Glib::Dispatcher GtkmmMediaPlayer::m_dispatcher;

GtkmmMediaPlayer::GtkmmMediaPlayer(boost::shared_ptr<PlayList> playList)
    : MediaPlayer(playList)
{
    MediaPlayerThreadNotification::setCallback(&notifyGuiThread);
    m_dispatcher.connect(sigc::mem_fun(this, &MediaPlayer::processEventQueue));

    // Emit signal_button_press_event for popup menu:
    add_events(Gdk::BUTTON_PRESS_MASK);

    set_app_paintable(true);
    set_double_buffered(false);
}

void GtkmmMediaPlayer::notifyGuiThread()
{
    m_dispatcher();
}

void GtkmmMediaPlayer::process(boost::shared_ptr<NotificationFileInfo> event)
{
    notificationFileName(event->fileName);
    notificationDuration(event->duration);
    notificationCurrentTime(0);
}

void GtkmmMediaPlayer::process(boost::shared_ptr<NotificationCurrentTime> event)
{
    notificationCurrentTime(event->time);
}

void GtkmmMediaPlayer::process(boost::shared_ptr<NotificationCurrentVolume> event)
{
    notificationCurrentVolume(*event);
}

void GtkmmMediaPlayer::process(boost::shared_ptr<ResizeVideoOutputReq> event)
{
    notificationResizeOutput(*event);
}

void GtkmmMediaPlayer::on_realize()
{
    INFO();

    //Call base class:
    Gtk::DrawingArea::on_realize();

    Display* xdisplay = get_x_display(*this);
    XID      xid = get_x_window(*this);

    videoOutput->queue_event(boost::make_shared<WindowRealizeEvent>(xdisplay, xid));

    open();
}

bool GtkmmMediaPlayer::on_configure_event(GdkEventConfigure* event)
{
    INFO();

    if (videoOutput)
    {
	videoOutput->queue_event(boost::make_shared<WindowConfigureEvent>
				 (event->x, event->y, event->width, event->height));
    }

    return Gtk::DrawingArea::on_configure_event(event);
}

bool GtkmmMediaPlayer::on_expose_event(GdkEventExpose* event)
{
    INFO();

    if (videoOutput &&
	event->count == 0)  // The last Expose event.
    {
	videoOutput->queue_event(boost::make_shared<WindowExposeEvent>());
    }

    return Gtk::DrawingArea::on_expose_event(event);
}

