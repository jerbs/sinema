//
// gtkmm Media Player
//
// Copyright (C) Joachim Erbs, 2009
//

#include "gui/GtkmmMediaPlayer.hpp"
#include "platform/timer.hpp"
#include "player/VideoOutput.hpp"

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

GtkmmMediaPlayer::GtkmmMediaPlayer(PlayList& playList)
    : MediaPlayer(playList),
      m_blankGdkCursor(gdk_cursor_new(GDK_BLANK_CURSOR)),
      m_blankCursor(Gdk::Cursor(m_blankGdkCursor))
{
    MediaPlayerThreadNotification::setCallback(&notifyGuiThread);
    m_dispatcher.connect(sigc::mem_fun(this, &MediaPlayer::processEventQueue));

    // Get X event signals for mouse motion, mouse button press and 
    // release events. For mouse button release events no event_handler 
    // is implemented, but without adding this event the mouse pointer 
    // isn't set back to default, when leaving the GtkmmMediaPlayer 
    // widget after a mouse button press event.
    add_events(Gdk::POINTER_MOTION_MASK);
    add_events(Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);
    add_events(Gdk::KEY_PRESS_MASK|Gdk::KEY_RELEASE_MASK);

    set_app_paintable(true);
    set_double_buffered(false);
    set_flags(Gtk::CAN_FOCUS);

    m_hideCursorTimer.relative(getTimespec(2));

    // Setup drop destination:
    std::list<Gtk::TargetEntry> targetList;
    targetList.push_back(Gtk::TargetEntry("text/uri-list"));
    drag_dest_set(targetList);
}

GtkmmMediaPlayer::~GtkmmMediaPlayer()
{
    if (m_blankGdkCursor)
    {
	gdk_cursor_unref(m_blankGdkCursor);
    }
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

void GtkmmMediaPlayer::process(boost::shared_ptr<NotificationVideoSize> event)
{
    notificationVideoSize(*event);
}

void GtkmmMediaPlayer::process(boost::shared_ptr<NotificationClipping> event)
{
    notificationClipping(*event);
}

void GtkmmMediaPlayer::process(boost::shared_ptr<NotificationDeinterlacerList> event)
{
    DEBUG();
    notificationDeinterlacerList(*event);
}

void GtkmmMediaPlayer::process(boost::shared_ptr<CloseFileResp> event)
{
    notificationCurrentTime(0);
    notificationFileClosed();
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

    // Mouse cursor:
    Glib::RefPtr <Gdk::Window> gdkWindow = get_window();
    if (gdkWindow)
    {
	gdkWindow->set_cursor(m_blankCursor);
    }
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

template<class EVENT>
void GtkmmMediaPlayer::getQuadrant(EVENT* event, int&x, int&y)
{
    x = 1;
    y = 1;
    if (3 * event->x <     get_width()) x = 0;
    if (3 * event->x > 2 * get_width()) x = 2;
    if (3 * event->y <     get_height()) y = 0;
    if (3 * event->y > 2 * get_height()) y = 2;
}

bool GtkmmMediaPlayer::on_motion_notify_event(GdkEventMotion* event)
{
    const Gdk::CursorType cursorType[3][3] =
	{ {Gdk::TOP_LEFT_CORNER,    Gdk::TOP_SIDE,    Gdk::TOP_RIGHT_CORNER},
	  {Gdk::LEFT_SIDE,          Gdk::TCROSS,      Gdk::RIGHT_SIDE},
	  {Gdk::BOTTOM_LEFT_CORNER, Gdk::BOTTOM_SIDE, Gdk::BOTTOM_RIGHT_CORNER} };
    Glib::RefPtr <Gdk::Window> gdkWindow = get_window();
    if (gdkWindow)
    {
	int x, y;
	getQuadrant(event, x, y);
	m_cursor = Gdk::Cursor(cursorType[y][x]);
	gdkWindow->set_cursor(m_cursor);

	start_timer(boost::make_shared<HideCursorEvent>(), m_hideCursorTimer);
    }

    grab_focus();

    // Event has been handled:
    return true;
}

bool GtkmmMediaPlayer::on_button_press_event(GdkEventButton* event)
{
    if(event->type == GDK_BUTTON_PRESS)
    {
	if (event->button == 1)
	{
	    ClipVideoDstEvent* clipVideoDstEventPtr;
	    int x, y;
	    getQuadrant(event, x, y);
	    if (x == 1 && y == 1)
	    {
		clipVideoDstEventPtr = ClipVideoDstEvent::createDisable();
	    }
	    else
	    {
		clipVideoDstEventPtr = ClipVideoDstEvent::createKeepIt();
		if (x == 0) clipVideoDstEventPtr->left = event->x;
		if (x == 2) clipVideoDstEventPtr->right = event->x;
		if (y == 0) clipVideoDstEventPtr->top = event->y;
		if (y == 2) clipVideoDstEventPtr->bottom = event->y;
	    }
	    boost::shared_ptr<ClipVideoDstEvent> clipVideoDstEvent(clipVideoDstEventPtr);
	    videoOutput->queue_event(clipVideoDstEvent);

	    // Event has been handled:
	    return true;
	}
    }

    // Event has not been handled:
    return false;
}

void GtkmmMediaPlayer::process(boost::shared_ptr<HideCursorEvent> event)
{
    Glib::RefPtr <Gdk::Window> gdkWindow = get_window();
    if (gdkWindow)
    {
	gdkWindow->set_cursor(m_blankCursor);
    }
}
