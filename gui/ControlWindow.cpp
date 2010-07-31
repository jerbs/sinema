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

#include <iostream>
#include <iomanip>

#include "gui/ControlWindow.hpp"
#include "gui/General.hpp"
#include "gui/SignalDispatcher.hpp"

ControlWindow::ControlWindow(SignalDispatcher& signalDispatcher)
    : Gtk::Window(),
      m_HBox_Level0(false, 2),  // not homogeneous, spacing
      m_VBox_Level1(false, 2), 
      m_HBox_Level2(false, 10),
      m_ScrollbarPosition(signalDispatcher.getPositionAdjustment()),
      m_VScaleVolume(signalDispatcher.getVolumeAdjustment()),
      m_Mute(),
      m_shown(false),
      m_pos_x(0),
      m_pos_y(0)
{
    set_title(applicationName + " (Ctrl)");
    set_default_size(400,0);

    add(m_HBox_Level0);

    m_HBox_Level0.pack_start(m_VBox_Level1);

    m_VBox_Level1.pack_start(*signalDispatcher.getCtrlWinToolBarWidget(), Gtk::PACK_EXPAND_WIDGET);

    m_LabelTitle.set_text("Title");
    m_VBox_Level1.pack_start(m_LabelTitle);
    m_VBox_Level1.pack_start(m_HBox_Level2);

    m_LabelTime.set_text("00:00:00");
    m_LabelDuration.set_text("00:00:00");    
    m_HBox_Level2.pack_start(m_LabelTime);
    m_HBox_Level2.pack_start(m_LabelDuration);

    m_VBox_Level1.pack_end(m_ScrollbarPosition, Gtk::PACK_SHRINK);

    // Don't display value as string next to the slider
    m_VScaleVolume.set_draw_value(false);
    // Lowest value at bottom, highest at top:
    m_VScaleVolume.set_inverted(true);
    m_HBox_Level0.pack_end(m_VBox_Level1a, Gtk::PACK_SHRINK);

    m_VBox_Level1a.pack_start(m_VScaleVolume, Gtk::PACK_EXPAND_WIDGET);
    m_VBox_Level1a.pack_end(m_Mute, Gtk::PACK_SHRINK);

    // Clear text again, that was set when connecting the CheckButton to the Action:
    m_Mute.set_label("");


    show_all_children();
}

ControlWindow::~ControlWindow()
{
}

void ControlWindow::on_show_window(bool pshow)
{
    if (pshow)
    {
	// Let the window manager decide where to place the window when shown
	// for the first time. Use previous position when shown again.
	if (m_shown)
	{
	    // The window manager may ignore or modify the move request.
	    // Partially visible windows are for example replaced by KDE.
	    move(m_pos_x, m_pos_y);
	}
	show();
	raise();
	deiconify();
	m_shown = true;
    }
    else
    {
	get_position(m_pos_x, m_pos_y);
	hide();
    }
}

void ControlWindow::on_set_title(Glib::ustring title)
{
    Gtk::Window::set_title(applicationName + " (Ctrl) " + title);
    m_LabelTitle.set_text(title);
}

std::string getTimeString(int seconds)
{
    int a = seconds/60;
    int h = a/60;
    int m = a - 60*h;
    int s = seconds - 3600*h - 60*m;

    std::stringstream ss;
    ss << std::setfill('0') 
       << std::setw(2) << h << ":"
       << std::setw(2) << m << ":"
       << std::setw(2) << s;

    return ss.str();
}

void ControlWindow::on_set_time(double seconds)
{
    m_LabelTime.set_text(getTimeString(seconds));
}

void ControlWindow::on_set_duration(double seconds)
{
    m_LabelDuration.set_text(getTimeString(seconds));
}
