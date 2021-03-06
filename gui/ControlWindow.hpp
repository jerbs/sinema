//
// Control Window
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

class SignalDispatcher;
class NotificationCurrentVolume;

class ControlWindow : public Gtk::Window
{
public:
    ControlWindow(SignalDispatcher& signalDispatcher);
    virtual ~ControlWindow();

    void on_set_title(Glib::ustring title);
    void on_set_duration(double seconds);
    void on_set_time(double seconds);
    void on_show_window(bool show);

private:
    // Child widgets:
    Gtk::HBox m_HBox_Level0;
    Gtk::VBox m_VBox_Level1;
    Gtk::VBox m_VBox_Level1a;
    Gtk::HBox m_HBox_Level2;

    Gtk::Label m_LabelTitle;
    Gtk::Label m_LabelTime;
    Gtk::Label m_LabelDuration;

    Gtk::HScrollbar m_ScrollbarPosition;

    Gtk::VScale m_VScaleVolume;
    Gtk::CheckButton m_Mute;

    bool m_shown;
    int m_pos_x;
    int m_pos_y;
};

#endif
