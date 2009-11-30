//
// Main Window
//
// Copyright (C) Joachim Erbs, 2009
//

#include <iostream>

#include "gui/MainWindow.hpp"
#include "gui/SignalDispatcher.hpp"

#undef DEBUG
#define DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

MainWindow::MainWindow(GtkmmMediaPlayer& gtkmmMediaPlayer,
		       SignalDispatcher& signalDispatcher)
    : Gtk::Window(),
      m_GtkmmMediaPlayer(gtkmmMediaPlayer),
      m_video_width(0),
      m_video_height(0),
      m_video_zoom(1),
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

    show_all_children();
}

MainWindow::~MainWindow()
{
}

void MainWindow::on_hide_window()
{
    hide();
}

void MainWindow::on_notification_video_size(const NotificationVideoSize& event)
{
    NotificationVideoSize oldVideoSize = m_VideoSize;
    m_VideoSize = event;

    DEBUG( << std::dec
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

    // Trying to resize window as expected by user.
    if ( ( (oldVideoSize.widthDst != m_VideoSize.widthDst) ||
	   (oldVideoSize.heightDst != m_VideoSize.heightDst) ) &&
	 (oldVideoSize.widthSrc == m_VideoSize.widthSrc) &&
	 (oldVideoSize.heightSrc == m_VideoSize.heightSrc) )
    {
	// User resized the window. Update zoom factor:
	m_video_zoom = xzoom;
    }
    else
    {
	// New video or clipping modified.
	// Keep zoom factor and resize window.
    }

    zoom(m_video_zoom);
}
    
void MainWindow::zoom(double percent)
{
    m_video_zoom = percent;

    int video_width_zoomed  = m_VideoSize.widthSrc  * m_video_zoom;
    int video_height_zoomed = m_VideoSize.heightSrc * m_video_zoom;

    // MainWindow displays Video and optionally some control elements.
    int window_width  = video_width_zoomed  + get_width()  - m_GtkmmMediaPlayer.get_width();
    int window_height = video_height_zoomed + get_height() - m_GtkmmMediaPlayer.get_height();

    DEBUG( << std::dec 
	   << window_width << " = " << video_width_zoomed << " + " << get_width() << " - " << m_GtkmmMediaPlayer.get_width() );
    DEBUG( << window_height << " = " << video_height_zoomed << " + " << get_height()<< " - " << m_GtkmmMediaPlayer.get_height());

    resize(window_width, window_height);
}
