//
// Main Window
//
// Copyright (C) Joachim Erbs, 2009
//

#include <iostream>

#include "platform/timer.hpp"
#include "gui/MainWindow.hpp"
#include "gui/SignalDispatcher.hpp"

#undef DEBUG
#define DEBUG(s)
// #define DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

MainWindow::MainWindow(GtkmmMediaPlayer& gtkmmMediaPlayer,
		       SignalDispatcher& signalDispatcher)
    : Gtk::Window(),
      m_GtkmmMediaPlayer(gtkmmMediaPlayer),
      m_video_zoom(1),
      m_fullscreen(false),
      m_ignore_window_size_change(getTimespec(0)),
      m_StatusBar(signalDispatcher.getStatusBar())
{
    set_title("player");
    set_default_size(400, 300);

    // Hot Keys:
    Glib::RefPtr<Gtk::UIManager> refUIManager = signalDispatcher.getUIManager();
    add_accel_group(refUIManager->get_accel_group());

    // Menu Bar:
    Gtk::Widget* pMenubar = signalDispatcher.getMenuBarWidget();
    if(pMenubar)
    {
	m_Box.pack_start(*pMenubar, Gtk::PACK_SHRINK);
    }

    // Tool Bar:
    Gtk::Widget* pToolbar = signalDispatcher.getToolBarWidget();
    if(pToolbar)
    {
	m_Box.pack_start(*pToolbar, Gtk::PACK_SHRINK);
    }

    // Main Area:
    m_Box.pack_start(m_GtkmmMediaPlayer, Gtk::PACK_EXPAND_WIDGET);

    // Status Bar:
    m_Box.pack_end(m_StatusBar, Gtk::PACK_SHRINK);

    add(m_Box);

    signalDispatcher.hideMainWindow.connect( sigc::mem_fun(*this, &MainWindow::on_hide_window) );
    signal_window_state_event().connect(sigc::mem_fun(*this, &MainWindow::on_main_window_state_event));

    show_all_children();
}

MainWindow::~MainWindow()
{
}

void MainWindow::on_hide_window()
{
    hide();
}

void MainWindow::ignoreWindowResize()
{
    // Ignore NotificationVideoSize(WindowSizeChanged) events received 
    // in the next 0.1 seconds:
    m_ignore_window_size_change = timer::get_current_time() + getTimespec(0.1);
}

bool MainWindow::on_main_window_state_event(GdkEventWindowState* event)
{
    // This method is called when fullscreen mode is entered or left. This
    // may be triggered by the application itself or by the window manager.

    bool fullscreen = event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN;
    if (fullscreen != m_fullscreen)
    {
	ignoreWindowResize();
    }
    m_fullscreen = fullscreen;
    DEBUG("m_fullscreen = " << m_fullscreen);

    return false;
}

std::ostream& operator<<(std::ostream& strm, const NotificationVideoSize::Reason& reason)
{
    switch (reason)
    {
    case NotificationVideoSize::VideoSizeChanged:
	strm << "VidSizeChanged";
	break;
    case NotificationVideoSize::WindowSizeChanged:
	strm << "WinSizeChanged";
	break;
    case NotificationVideoSize::ClippingChanged:
	strm << "ClippingModified";
	break;
    default:
	strm << int(reason);
	break;
    }

    return strm;
}

void MainWindow::on_notification_video_size(const NotificationVideoSize& event)
{
    m_VideoSize = event;

    DEBUG( << std::dec << event.reason << ","
	   << "vid(" << m_VideoSize.widthVid << "," << m_VideoSize.heightVid << ")"
	   << "win(" << m_VideoSize.widthWin << "," << m_VideoSize.heightWin << ")"
	   << "dst(" << m_VideoSize.widthDst << "," << m_VideoSize.heightDst << ")"
	   << "src(" << m_VideoSize.widthSrc << "," << m_VideoSize.heightSrc << ")" );

    const double xzoom = round(double(m_VideoSize.widthDst)  / double(m_VideoSize.widthSrc) * 100) / 100;
    const double yzoom = round(double(m_VideoSize.heightDst) / double(m_VideoSize.heightSrc)* 100) / 100;

    std::stringstream ss;

    if (xzoom == yzoom)
    {
	ss << xzoom * 100 << " %";
    }
    else
    {
	ss << xzoom * 100 << " % x " << yzoom * 100 << " %";
    }
    m_StatusBar.push(ss.str(), 0);

    switch(event.reason)
    {
    case NotificationVideoSize::VideoSizeChanged:
	m_video_zoom = 1;
	zoom(m_video_zoom);
	break;

    case NotificationVideoSize::WindowSizeChanged:
	if (timer::get_current_time() < m_ignore_window_size_change)
	{
	    // Don't use the first NotificationVideoSize(WindowSizeChanged) events
	    // after leaving the fullscreen mode to change the zoom factor.
	    zoom(m_video_zoom);
	}
	else
	{
	    m_video_zoom = xzoom;
	    zoom(m_video_zoom);
	}
	break;

    case NotificationVideoSize::ClippingChanged:
	zoom(m_video_zoom);
	break;
    }
}
    
void MainWindow::zoom(double percent)
{
    m_video_zoom = percent;

    int video_width_zoomed  = m_VideoSize.widthSrc  * m_video_zoom;
    int video_height_zoomed = m_VideoSize.heightSrc * m_video_zoom;

    // MainWindow displays Video and optionally some control elements.
    int window_width  = video_width_zoomed  + get_width()  - m_GtkmmMediaPlayer.get_width();
    int window_height = video_height_zoomed + get_height() - m_GtkmmMediaPlayer.get_height();

    DEBUG( << std::dec << 100 * m_video_zoom << "%" );
    DEBUG( << window_width << " = " << video_width_zoomed << " + " << get_width() << " - " << m_GtkmmMediaPlayer.get_width() );
    DEBUG( << window_height << " = " << video_height_zoomed << " + " << get_height()<< " - " << m_GtkmmMediaPlayer.get_height());

    resize(window_width, window_height);
}
