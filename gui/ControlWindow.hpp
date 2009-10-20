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
#include <gtkmm/checkbutton.h>
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
    virtual void on_position_changed();
    virtual void on_position_value_changed();
    virtual void on_volume_changed();
    virtual void on_volume_value_changed();
    virtual void on_mute_toggled();

    void set_title(Glib::ustring title);
    void set_duration(double seconds);
    void set_time(double seconds);
    void set_volume(NotificationCurrentVolume vol);

    GtkmmMediaPlayer& m_MediaPlayer;
    bool acceptAdjustmentPositionValueChanged;
    bool acceptAdjustmentVolumeValueChanged;

    // Child widgets:
    Gtk::HBox m_HBox_Level0;
    Gtk::VBox m_VBox_Level1;
    Gtk::VBox m_VBox_Level1a;
    Gtk::HBox m_HBox_Level2;
    Gtk::VBox m_VBox_Level3;
    Gtk::HBox m_HBox_Level4;

    Gtk::Table m_Table;
    Gtk::Button m_Play, m_Pause, m_Stop;
    Gtk::Button m_Prev, m_Next, m_Rewind, m_Forward;

    Gtk::Label m_LabelTitle;
    Gtk::Label m_LabelTime;
    Gtk::Label m_LabelDuration;

    Gtk::Adjustment m_AdjustmentPosition;
    Gtk::HScrollbar m_ScrollbarPosition;

    Gtk::Adjustment m_AdjustmentVolume;
    Gtk::VScale m_VScaleVolume;
    Gtk::CheckButton m_Mute;
};

#endif
