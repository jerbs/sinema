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
      m_pMenuPopup(0)
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

    // Popup Menu
    m_pMenuPopup = signalDispatcher.getPopupMenuWidget();
    if(!m_pMenuPopup)
    {
	g_warning("menu not found");
    }

    // Main Area:
    m_GtkmmMediaPlayer.signal_button_press_event().connect(sigc::mem_fun(*this,
				   &MainWindow::on_button_press_event) );
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

bool MainWindow::on_button_press_event(GdkEventButton* event)
{
    INFO();
    if( (event->type == GDK_BUTTON_PRESS) && (event->button == 3) )
    {
	if(m_pMenuPopup)
	    m_pMenuPopup->popup(event->button, event->time);

	// Event has been handled:
	return true;
    }
    else
    {
	// Event has not been handled:
	return false;
    }
}

void MainWindow::on_hide_window()
{
    hide();
}

void MainWindow::on_resize_video_output(const ResizeVideoOutputReq& size)
{
    // MainWindow displays Video and optionally some control elements.
    int width  = size.width  + get_width()  - m_GtkmmMediaPlayer.get_width();
    int height = size.height + get_height() - m_GtkmmMediaPlayer.get_height();

    resize(width, height);
}
