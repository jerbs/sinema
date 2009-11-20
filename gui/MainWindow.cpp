//
// Main Window
//
// Copyright (C) Joachim Erbs, 2009
//

#include <iostream>

#include "gui/MainWindow.hpp"
#include "gui/SignalDispatcher.hpp"

MainWindow::MainWindow(GtkmmMediaPlayer& gtkmmMediaPlayer,
		       SignalDispatcher& signalDispatcher)
    : Gtk::Window(),
      m_GtkmmMediaPlayer(gtkmmMediaPlayer),
      m_video_width(0),
      m_video_height(0),
      m_video_zoom(100)
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
    m_GtkmmMediaPlayer.notificationResizeOutput.connect(sigc::mem_fun(*this,
				   &MainWindow::on_resize_video_output) );
    m_Box.pack_start(m_GtkmmMediaPlayer, Gtk::PACK_EXPAND_WIDGET);

    // Status Bar:
    m_Box.pack_end(signalDispatcher.getStatusBar(), Gtk::PACK_SHRINK);

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

void MainWindow::on_resize_video_output(const ResizeVideoOutputReq& size)
{
    m_video_width = size.width;
    m_video_height = size.height;
    zoom(m_video_zoom);
}

void MainWindow::zoom(int percent)
{
    m_video_zoom = percent;

    int video_width_zoomed  = double(m_video_width  * m_video_zoom) / 100;
    int video_height_zoomed = double(m_video_height * m_video_zoom) / 100;

    // MainWindow displays Video and optionally some control elements.
    int window_width  = video_width_zoomed  + get_width()  - m_GtkmmMediaPlayer.get_width();
    int window_height = video_height_zoomed + get_height() - m_GtkmmMediaPlayer.get_height();

    resize(window_width, window_height);
}
