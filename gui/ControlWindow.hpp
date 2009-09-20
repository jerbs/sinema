//
// Control Window
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef CONTROL_WINDOW_HPP
#define CONTROL_WINDOW_HPP

#include <gtkmm/window.h>
#include <gtkmm/table.h>
#include <gtkmm/button.h>

#include "player/MediaPlayer.hpp"

class ControlWindow : public Gtk::Window
{
public:
    ControlWindow(MediaPlayer& mediaPlayer);
    virtual ~ControlWindow();

protected:
    // Signal handlers:
    virtual void on_button_play();
    virtual void on_button_pause();
    virtual void on_button_stop();
    virtual void on_button_prev();
    virtual void on_button_next();
    virtual void on_button_rewind();
    virtual void on_button_forward();

    MediaPlayer& m_MediaPlayer;

    // Child widgets:
    Gtk::Table m_Table;
    Gtk::Button m_Play, m_Pause, m_Stop;
    Gtk::Button m_Prev, m_Next, m_Rewind, m_Forward;
};

#endif
