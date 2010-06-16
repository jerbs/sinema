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
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/scale.h>
#include <glibmm/dispatcher.h>
#include <glibmm/ustring.h>

#include "gui/GtkmmMediaPlayer.hpp"

class ControlWindow : public Gtk::Window
{
public:
    ControlWindow(GtkmmMediaPlayer& mediaPlayer);
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

    void set_title(Glib::ustring title);
    void set_time(int seconds);

    GtkmmMediaPlayer& m_MediaPlayer;

    // Child widgets:
    Gtk::HBox m_HBox_Level0;
    Gtk::VBox m_VBox_Level1;
    Gtk::HBox m_HBox_Level2;
    Gtk::VBox m_VBox_Level3;

    Gtk::Table m_Table;
    Gtk::Button m_Play, m_Pause, m_Stop;
    Gtk::Button m_Prev, m_Next, m_Rewind, m_Forward;

    Gtk::Label m_LabelTitle;
    Gtk::Label m_LabelTime;

    Gtk::Adjustment m_AdjustmentPosition;
    Gtk::HScrollbar m_ScrollbarPosition;

    Gtk::Adjustment m_AdjustmentVolume;
    Gtk::VScale m_VScaleVolume;

    // Dispatcher
    Glib::Dispatcher m_dispatcherTitle;
    Glib::Dispatcher m_dispatcherTime;
};

#endif
