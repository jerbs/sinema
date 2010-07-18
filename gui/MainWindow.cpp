//
// Main Window
//
// Copyright (C) Joachim Erbs, 2009
//

#include <iostream>

#include "platform/timer.hpp"
#include "gui/MainWindow.hpp"
#include "gui/SignalDispatcher.hpp"

std::string applicationName = "player";

MainWindow::MainWindow(GtkmmMediaPlayer& gtkmmMediaPlayer,
		       SignalDispatcher& signalDispatcher)
    : Gtk::Window(),
      m_GtkmmMediaPlayer(gtkmmMediaPlayer),
      m_StatusBar(signalDispatcher.getStatusBar()),
      m_ScrollbarPosition(signalDispatcher.getPositionAdjustment()),
      m_video_zoom(1),
      m_fullscreen(false),
      m_ignore_window_size_change(getTimespec(0))
{
    set_title(applicationName);
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
    m_StatusBar.set_spacing(15);
    m_StatusBar.pack_end(m_LabelStatusBarDuration, Gtk::PACK_SHRINK);
    m_StatusBar.pack_end(m_LabelStatusBarTime, Gtk::PACK_SHRINK);
    m_StatusBar.pack_end(m_LabelStatusBarSize, Gtk::PACK_SHRINK);
    m_StatusBar.pack_end(m_LabelStatusBarZoom, Gtk::PACK_SHRINK);
    m_StatusBar.pack_end(m_ScrollbarPosition);
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
    TRACE_DEBUG("m_fullscreen = " << m_fullscreen);

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

    TRACE_DEBUG( << std::dec << event.reason << ","
		 << "vid(" << m_VideoSize.widthVid << "," << m_VideoSize.heightVid << ")"
		 << "win(" << m_VideoSize.widthWin << "," << m_VideoSize.heightWin << ")"
		 << "dst(" << m_VideoSize.widthDst << "," << m_VideoSize.heightDst << ")"
		 << "src(" << m_VideoSize.widthSrc << "," << m_VideoSize.heightSrc << ")"
		 << "adj(" << m_VideoSize.widthAdj << "," << m_VideoSize.heightAdj << ")" );

    const double xzoom = round(double(m_VideoSize.widthDst)  / double(m_VideoSize.widthAdj) * 100) / 100;
    // const double yzoom = round(double(m_VideoSize.heightDst) / double(m_VideoSize.heightAdj)* 100) / 100;

    std::stringstream ss;
    ss << xzoom * 100 << " %";
    m_LabelStatusBarZoom.set_text(ss.str());

    ss.clear();  // Clear the state of the ios object.
    ss.str("");  // Erase the content of the underlying string object.
    ss << m_VideoSize.widthVid << " x " << m_VideoSize.heightVid;
    m_LabelStatusBarSize.set_text(ss.str());

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

extern std::string getTimeString(int seconds);

void MainWindow::set_duration(double seconds)
{
    m_LabelStatusBarDuration.set_text(getTimeString(seconds));
}

void MainWindow::set_time(double seconds)
{
    m_LabelStatusBarTime.set_text(getTimeString(seconds));
}

void MainWindow::set_title(Glib::ustring title)
{
    Gtk::Window::set_title(applicationName +" - " + title);
}

void MainWindow::zoom(double percent)
{
    m_video_zoom = percent;

    int video_width_zoomed  = m_VideoSize.widthAdj  * m_video_zoom;
    int video_height_zoomed = m_VideoSize.heightAdj * m_video_zoom;

    // MainWindow displays Video and optionally some control elements.
    int window_width  = video_width_zoomed  + get_width()  - m_GtkmmMediaPlayer.get_width();
    int window_height = video_height_zoomed + get_height() - m_GtkmmMediaPlayer.get_height();

    TRACE_DEBUG( << std::dec << 100 * m_video_zoom << "%" );
    TRACE_DEBUG( << window_width << " = " << video_width_zoomed << " + " << get_width() << " - " << m_GtkmmMediaPlayer.get_width() );
    TRACE_DEBUG( << window_height << " = " << video_height_zoomed << " + " << get_height()<< " - " << m_GtkmmMediaPlayer.get_height());

    resize(window_width, window_height);
}
