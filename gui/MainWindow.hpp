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
#include "gui/GtkmmMediaPlayer.hpp"

class SignalDispatcher;

class MainWindow : public Gtk::Window
{
public:
    MainWindow(GtkmmMediaPlayer& m_GtkmmMediaPlayer,
	       SignalDispatcher& signalDispatcher);
    virtual ~MainWindow();

private:
    virtual void on_hide_window();
    virtual void on_resize_video_output(const ResizeVideoOutputReq& size);

    //Child widgets:
    Gtk::VBox m_Box;
    Gtk::Label m_Label;
    Gtk::Menu* m_pMenuPopup;
    GtkmmMediaPlayer& m_GtkmmMediaPlayer;
};

#endif
