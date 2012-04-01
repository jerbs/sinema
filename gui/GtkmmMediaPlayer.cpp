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

#include "gui/GtkmmMediaPlayer.hpp"
#include "platform/timer.hpp"
#include "player/VideoOutput.hpp"

#include <boost/bind.hpp>
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

GtkmmMediaPlayer::GtkmmMediaPlayer(PlayList& playList)
    : GlibmmEventDispatcher<MediaPlayer>(playList),
      m_blankGdkCursor(gdk_cursor_new(GDK_BLANK_CURSOR)),
      m_blankCursor(Gdk::Cursor(m_blankGdkCursor)),
      m_fullscreen(false),
      m_video_width(0),
      m_video_height(0),
      m_video_zoom(1),
      m_zoom_enabled(true)
{
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
    TRACE_DEBUG( << *event);

    m_video_width = event->widthAdj;
    m_video_height = event->heightAdj;

    switch(event->reason)
    {
    case NotificationVideoSize::VideoSizeChanged:
	zoom(1);
	break;

    case NotificationVideoSize::WindowSizeChanged:
	// ignore this event.
	break;

    case NotificationVideoSize::ClippingChanged:
	dontZoom();
	break;
    }

    notificationVideoSize(*event);
}

bool GtkmmMediaPlayer::on_main_window_state_event(GdkEventWindowState* event)
{
    // This method is called when fullscreen mode is entered or left. This
    // may be triggered by the application itself or by the window manager.

    m_fullscreen = event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN;
    TRACE_DEBUG("m_fullscreen = " << m_fullscreen);

    return false;
}

void GtkmmMediaPlayer::on_size_allocate(Gtk::Allocation& allocation)
{
    Gtk::DrawingArea::on_size_allocate(allocation);

    int width =allocation.get_width();
    int height = allocation.get_height();

    TRACE_DEBUG( << "(" << width << "," << height << ")");

    if (m_zoom_enabled && !m_fullscreen)
    {
	// Here the user resized the window with the window manager.
	// Calculate the new zoom factor.

	const double xzoom = round(double(width)  / double(m_video_width) * 100) / 100;
	// const double yzoom = round(double(height) / double(m_video_height)* 100) / 100;

	TRACE_DEBUG( << "zoom: " << m_video_zoom * 100 << "% -> " << xzoom * 100 << "%");
	m_video_zoom = xzoom;
    }
}

void GtkmmMediaPlayer::dontZoom()
{
    TRACE_DEBUG();

    m_zoom_enabled = false;
    resizeWindow();

    Glib::signal_idle().connect( sigc::mem_fun(*this, &GtkmmMediaPlayer::on_idle) );
}

void GtkmmMediaPlayer::zoom(double percent)
{
    // Change zoom factor and window size.

    TRACE_DEBUG( << "zoom: " << m_video_zoom * 100 << "% -> " << percent * 100 << "%");
    m_video_zoom = percent;
    resizeWindow();
}

void GtkmmMediaPlayer::resizeWindow()
{
    int video_width_zoomed  = m_video_width  * m_video_zoom;
    int video_height_zoomed = m_video_height * m_video_zoom;

    TRACE_DEBUG( << "size request: (" << video_width_zoomed << "," << video_height_zoomed << ")");

    set_size_request(video_width_zoomed, video_height_zoomed);
    resizeMainWindow();

    Glib::signal_idle().connect( sigc::mem_fun(*this, &GtkmmMediaPlayer::on_idle) );
}

bool GtkmmMediaPlayer::on_idle()
{
    TRACE_DEBUG();

    // Resize is finished. Allow user to shrink window again:
    set_size_request(-1,-1);

    m_zoom_enabled = true;

    // Remove idle callback:
    return false;
}

void GtkmmMediaPlayer::process(boost::shared_ptr<NotificationClipping> event)
{
    notificationClipping(*event);
}

void GtkmmMediaPlayer::process(boost::shared_ptr<NotificationDeinterlacerList> event)
{
    TRACE_DEBUG();
    notificationDeinterlacerList(*event);
}

void GtkmmMediaPlayer::process(boost::shared_ptr<NotificationVideoAttribute> event)
{
    TRACE_DEBUG();
    notificationVideoAttribute(*event);
}

void GtkmmMediaPlayer::process(boost::shared_ptr<CloseFileResp>)
{
    notificationCurrentTime(0);
    notificationFileClosed();
}

class GtkmmAddWindowSystemEventFilterFunctor: public AddWindowSystemEventFilterFunctor
{
public:
    GtkmmAddWindowSystemEventFilterFunctor()
    {}

    void operator()(boost::shared_ptr<WindowSystemEventFilterFunctor> filter)
    {
	// Don't know how to implement this with the gtkmm interface.
	// Using NULL as Gdk::Window point when calling the add_filter
	// method is not possible (segmentation fault).
	// With the Gdk::Window object used by the GtkmmMediaPlayer
	// object no events are received.
	gdk_window_add_filter (NULL, filter_cb, this);
	m_filter = filter;
    }

private:
    static GdkFilterReturn filter_cb(GdkXEvent* xevent,
				     GdkEvent* event,
				     gpointer data)
    {
	GtkmmAddWindowSystemEventFilterFunctor* obj = (GtkmmAddWindowSystemEventFilterFunctor*)data;
	if ((*obj->m_filter)(xevent))
	{
	    return GDK_FILTER_REMOVE;
	}
	return GDK_FILTER_CONTINUE;
    }

    boost::shared_ptr<WindowSystemEventFilterFunctor> m_filter;
};

void GtkmmMediaPlayer::on_realize()
{
    TRACE_DEBUG();

    //Call base class:
    Gtk::DrawingArea::on_realize();

    Display* xdisplay = get_x_display(*this);
    XID      xid = get_x_window(*this);

    videoOutput->queue_event(boost::make_shared<WindowRealizeEvent>(xdisplay, xid,
			     boost::make_shared<GtkmmAddWindowSystemEventFilterFunctor>()));

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
    TRACE_DEBUG();

    if (videoOutput)
    {
	videoOutput->queue_event(boost::make_shared<WindowConfigureEvent>
				 (event->x, event->y, event->width, event->height));
    }

    return Gtk::DrawingArea::on_configure_event(event);
}

bool GtkmmMediaPlayer::on_expose_event(GdkEventExpose* event)
{
    TRACE_DEBUG();

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

void GtkmmMediaPlayer::process(boost::shared_ptr<HideCursorEvent>)
{
    Glib::RefPtr <Gdk::Window> gdkWindow = get_window();
    if (gdkWindow)
    {
	gdkWindow->set_cursor(m_blankCursor);
    }
}
