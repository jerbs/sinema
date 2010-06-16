//
// Control Window
//
// Copyright (C) Joachim Erbs, 2009
//

#include <iostream>
#include <iomanip>

#include "gui/ControlWindow.hpp"

ControlWindow::ControlWindow(GtkmmMediaPlayer& mediaPlayer)
    : Gtk::Window(),
      m_MediaPlayer(mediaPlayer),
      m_HBox_Level0(false, 2),  // not homogeneous, spacing
      m_VBox_Level1(false, 2), 
      m_HBox_Level2(false, 10),
      m_VBox_Level3(false, 2), 
      m_Table(2, 4, true), // height, width, homogeneous
      m_Play("Play"),
      m_Pause("Pause"),
      m_Stop("Stop"),
      m_Prev("Prev"),
      m_Next("Next"),
      m_Rewind("RW"),
      m_Forward("FF"),
      m_AdjustmentPosition(0.0, 0.0, 101.0, 0.1, 1.0, 1.0),
      m_ScrollbarPosition(m_AdjustmentPosition),
      m_AdjustmentVolume(0.0, 0.0, 101.0, 0.1, 1.0, 1.0),
      m_VScaleVolume(m_AdjustmentVolume)
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
    m_LabelTime.set_text("00:00:00");
    m_VBox_Level3.pack_start(m_LabelTitle);
    m_VBox_Level3.pack_start(m_LabelTime);

    m_VBox_Level1.pack_end(m_ScrollbarPosition, Gtk::PACK_SHRINK);

    // Don't display value as string next to the slider
    m_VScaleVolume.set_draw_value(false);
    // Lowest value at buttom, highest at top:
    m_VScaleVolume.set_inverted(true);
    m_HBox_Level0.pack_end(m_VScaleVolume, Gtk::PACK_SHRINK);

    m_Play.signal_clicked().connect( sigc::mem_fun(*this, &ControlWindow::on_button_play) );
    m_Pause.signal_clicked().connect( sigc::mem_fun(*this, &ControlWindow::on_button_pause) );
    m_Stop.signal_clicked().connect( sigc::mem_fun(*this, &ControlWindow::on_button_stop) );
    m_Prev.signal_clicked().connect( sigc::mem_fun(*this, &ControlWindow::on_button_prev) );
    m_Next.signal_clicked().connect( sigc::mem_fun(*this, &ControlWindow::on_button_next) );
    m_Rewind.signal_clicked().connect( sigc::mem_fun(*this, &ControlWindow::on_button_rewind) );
    m_Forward.signal_clicked().connect( sigc::mem_fun(*this, &ControlWindow::on_button_forward) );

    m_MediaPlayer.notificationCurrentTitle.connect( sigc::mem_fun(*this, &ControlWindow::set_title) );
    m_MediaPlayer.notificationCurrentTime.connect( sigc::mem_fun(*this, &ControlWindow::set_time) );

    show_all_children();
}

ControlWindow::~ControlWindow()
{
}

void ControlWindow::on_button_play()
{
    m_MediaPlayer.open();
    m_MediaPlayer.play();
}

void ControlWindow::on_button_pause()
{
    m_MediaPlayer.pause();
}

void ControlWindow::on_button_stop()
{
    m_MediaPlayer.close();
}

void ControlWindow::on_button_prev()
{
    m_MediaPlayer.skipBack();
}

void ControlWindow::on_button_next()
{
    m_MediaPlayer.skipForward();
}

void ControlWindow::on_button_rewind()
{
}

void ControlWindow::on_button_forward()
{
}

void ControlWindow::set_title(Glib::ustring title)
{
    m_LabelTitle.set_text(title);
}

void ControlWindow::set_time(int seconds)
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
    m_LabelTime.set_text(ss.str());
}
