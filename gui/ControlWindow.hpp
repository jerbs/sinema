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
#include <gtkmm/scrollbar.h>
#include <gtkmm/scale.h>
#include <glibmm/ustring.h>

class GtkmmMediaPlayer;
class SignalDispatcher;
class NotificationCurrentVolume;

class ControlWindow : public Gtk::Window
{
public:
    ControlWindow(GtkmmMediaPlayer& mediaPlayer, SignalDispatcher& signalDispatcher);
    virtual ~ControlWindow();

private:
    virtual void on_show_control_window(bool show);

    void set_title(Glib::ustring title);
    void set_duration(double seconds);
    void set_time(double seconds);

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

    Gtk::HScrollbar m_ScrollbarPosition;

    Gtk::VScale m_VScaleVolume;
    Gtk::CheckButton m_Mute;
};

#endif
