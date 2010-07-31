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

#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <gtkmm/window.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/label.h>
#include <gtkmm/menu.h>
#include <gtkmm/toggleaction.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/statusbar.h>
#include "gui/GtkmmMediaPlayer.hpp"

class SignalDispatcher;

class MainWindow : public Gtk::Window
{
public:
    MainWindow(GtkmmMediaPlayer& m_GtkmmMediaPlayer,
	       SignalDispatcher& signalDispatcher);
    virtual ~MainWindow();

    virtual void on_notification_video_size(const NotificationVideoSize& event);
    void set_duration(double seconds);
    void set_time(double seconds);
    void set_title(Glib::ustring title);
    void zoom(double percent);
    void ignoreWindowResize();

private:
    virtual void on_hide_window();
    bool on_main_window_state_event(GdkEventWindowState* event);

    //Child widgets:
    Gtk::VBox m_Box;
    Gtk::Label m_Label;
    Gtk::Menu* m_pMenuPopup;
    GtkmmMediaPlayer& m_GtkmmMediaPlayer;
    Gtk::Statusbar& m_StatusBar;
    Gtk::Label m_LabelStatusBarDuration;
    Gtk::Label m_LabelStatusBarTime;
    Gtk::Label m_LabelStatusBarSize;
    Gtk::Label m_LabelStatusBarZoom;
    Gtk::HScrollbar m_ScrollbarPosition;

    NotificationVideoSize m_VideoSize;

    double m_video_zoom;
    bool m_fullscreen;
    timespec_t m_ignore_window_size_change;
};

#endif
