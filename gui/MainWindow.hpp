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
#include "player/MediaPlayer.hpp"

class SignalDispatcher;

class MainWindow : public Gtk::Window
{
public:
    MainWindow(SignalDispatcher& signalDispatcher);
    virtual ~MainWindow();

private:
    virtual bool on_button_press_event(GdkEventButton* event);
    virtual void on_hide_window();

    //Child widgets:
    Gtk::VBox m_Box;
    Gtk::EventBox m_EventBox;
    Gtk::Label m_Label;
    Gtk::Menu* m_pMenuPopup;
};

#endif
