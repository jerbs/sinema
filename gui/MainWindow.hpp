//
// Main Window
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/label.h>
#include <gtkmm/menu.h>
#include <gtkmm/toggleaction.h>
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

    NotificationVideoSize m_VideoSize;

    double m_video_zoom;
    bool m_fullscreen;
    timespec_t m_ignore_window_size_change;
};

#endif
