//
// Main Window
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

#include <iostream>

#include "platform/timer.hpp"
#include "gui/General.hpp"
#include "gui/MainWindow.hpp"
#include "gui/SignalDispatcher.hpp"

MainWindow::MainWindow(GtkmmMediaPlayer& gtkmmMediaPlayer,
		       SignalDispatcher& signalDispatcher)
    : Gtk::Window(),
      m_ScrollbarPosition(signalDispatcher.getPositionAdjustment())
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
    m_Box.pack_start(gtkmmMediaPlayer, Gtk::PACK_EXPAND_WIDGET);

    // Status Bar:
    Gtk::Statusbar& statusBar = signalDispatcher.getStatusBar();
    statusBar.set_spacing(15);
    statusBar.pack_end(m_LabelStatusBarDuration, Gtk::PACK_SHRINK);
    statusBar.pack_end(m_LabelStatusBarTime, Gtk::PACK_SHRINK);
    statusBar.pack_end(m_LabelStatusBarSize, Gtk::PACK_SHRINK);
    statusBar.pack_end(m_LabelStatusBarZoom, Gtk::PACK_SHRINK);
    statusBar.pack_end(m_ScrollbarPosition);
    m_Box.pack_end(statusBar, Gtk::PACK_SHRINK);

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

void MainWindow::resizeMainWindow()
{
    TRACE_DEBUG();
    // Start size requisition phase:
    resize(1,1);
}

void MainWindow::on_notification_video_size(const NotificationVideoSize& event)
{
    TRACE_DEBUG( << event );

    const double xzoom = round(double(event.widthDst)  / double(event.widthAdj) * 100) / 100;
    // const double yzoom = round(double(event.heightDst) / double(event.heightAdj)* 100) / 100;

    TRACE_DEBUG( << xzoom * 100 << " % "
		 << "(" << event.widthWin << "," << event.heightWin << ") "
		 << event.widthVid << " x " << event.heightVid );

    std::stringstream ss;
    ss << xzoom * 100 << " %";
    m_LabelStatusBarZoom.set_text(ss.str());

    ss.clear();  // Clear the state of the ios object.
    ss.str("");  // Erase the content of the underlying string object.
    ss << event.widthVid << " x " << event.heightVid;
    m_LabelStatusBarSize.set_text(ss.str());
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
