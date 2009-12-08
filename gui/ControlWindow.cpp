//
// Control Window
//
// Copyright (C) Joachim Erbs, 2009
//

#include <iostream>
#include <iomanip>

#include "gui/ControlWindow.hpp"
#include "gui/GtkmmMediaPlayer.hpp"
#include "gui/SignalDispatcher.hpp"

ControlWindow::ControlWindow(GtkmmMediaPlayer& mediaPlayer, SignalDispatcher& signalDispatcher)
    : Gtk::Window(),
      m_HBox_Level0(false, 2),  // not homogeneous, spacing
      m_VBox_Level1(false, 2), 
      m_HBox_Level2(false, 10),
      m_VBox_Level3(false, 2),
      m_HBox_Level4(false, 10),
      m_Table(2, 4, true), // height, width, homogeneous
      m_Play("Play"),
      m_Pause("Pause"),
      m_Stop("Stop"),
      m_Prev("Prev"),
      m_Next("Next"),
      m_Rewind("RW"),
      m_Forward("FF"),
      m_ScrollbarPosition(signalDispatcher.getPositionAdjustment()),
      m_VScaleVolume(signalDispatcher.getVolumeAdjustment()),
      m_Mute(),
      m_shown(false),
      m_pos_x(0),
      m_pos_y(0)
{
    set_title("Control Window");

    add(m_HBox_Level0);

    m_HBox_Level0.pack_start(m_VBox_Level1);

    m_VBox_Level1.pack_start(m_HBox_Level2, Gtk::PACK_SHRINK);

    m_HBox_Level2.pack_start(m_Table, Gtk::PACK_SHRINK);

    m_Table.attach(m_Play, 0,2, 0,1);
    m_Table.attach(m_Pause, 2,3, 0,1);
    m_Table.attach(m_Stop, 3,4, 0,1);
    m_Table.attach(m_Prev, 0,1, 1,2);
    m_Table.attach(m_Next, 1,2, 1,2);
    m_Table.attach(m_Rewind, 2,3, 1,2);
    m_Table.attach(m_Forward, 3,4, 1,2);

    m_HBox_Level2.pack_start(m_VBox_Level3);

    m_LabelTitle.set_text("Title");
    m_VBox_Level3.pack_start(m_LabelTitle);
    m_VBox_Level3.pack_start(m_HBox_Level4);

    m_LabelTime.set_text("00:00:00");
    m_LabelDuration.set_text("00:00:00");    
    m_HBox_Level4.pack_start(m_LabelTime);
    m_HBox_Level4.pack_start(m_LabelDuration);

    m_VBox_Level1.pack_end(m_ScrollbarPosition, Gtk::PACK_SHRINK);

    // Don't display value as string next to the slider
    m_VScaleVolume.set_draw_value(false);
    // Lowest value at bottom, highest at top:
    m_VScaleVolume.set_inverted(true);
    m_HBox_Level0.pack_end(m_VBox_Level1a, Gtk::PACK_SHRINK);

    m_VBox_Level1a.pack_start(m_VScaleVolume, Gtk::PACK_EXPAND_WIDGET);
    m_VBox_Level1a.pack_end(m_Mute, Gtk::PACK_SHRINK);

    Glib::RefPtr<Gtk::ActionGroup> actionGroup = signalDispatcher.getActionGroup();
    actionGroup->get_action("MediaPlay")->connect_proxy(m_Play);
    actionGroup->get_action("MediaPause")->connect_proxy(m_Pause);
    actionGroup->get_action("MediaStop")->connect_proxy(m_Stop);
    actionGroup->get_action("MediaPrevious")->connect_proxy(m_Prev);
    actionGroup->get_action("MediaNext")->connect_proxy(m_Next);
    actionGroup->get_action("MediaRewind")->connect_proxy(m_Rewind);
    actionGroup->get_action("MediaForward")->connect_proxy(m_Forward);
    actionGroup->get_action("ViewMute")->connect_proxy(m_Mute);

    // Clear text again, that was set when connecting the CheckButton to the Action:
    m_Mute.set_label("");

    mediaPlayer.notificationFileName.connect( sigc::mem_fun(*this, &ControlWindow::set_title) );
    mediaPlayer.notificationDuration.connect( sigc::mem_fun(*this, &ControlWindow::set_duration) );
    mediaPlayer.notificationCurrentTime.connect( sigc::mem_fun(*this, &ControlWindow::set_time) );

    signalDispatcher.showControlWindow.connect( sigc::mem_fun(*this, &ControlWindow::on_show_control_window) );

    show_all_children();
}

ControlWindow::~ControlWindow()
{
}

void ControlWindow::on_show_control_window(bool pshow)
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
	m_shown = true;
    }
    else
    {
	get_position(m_pos_x, m_pos_y);
	hide();
    }
}

void ControlWindow::set_title(Glib::ustring title)
{
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

void ControlWindow::set_time(double seconds)
{
    m_LabelTime.set_text(getTimeString(seconds));
}

void ControlWindow::set_duration(double seconds)
{
    m_LabelDuration.set_text(getTimeString(seconds));
}
