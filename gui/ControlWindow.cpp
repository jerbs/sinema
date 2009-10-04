//
// Control Window
//
// Copyright (C) Joachim Erbs, 2009
//

#include <iostream>
#include "gui/ControlWindow.hpp"

ControlWindow::ControlWindow(MediaPlayer& mediaPlayer)
    : Gtk::Window(),
      m_MediaPlayer(mediaPlayer),
      m_Table(2, 4, true), // homogeneous
      m_Play("Play"),
      m_Pause("Pause"),
      m_Stop("Stop"),
      m_Prev("Prev"),
      m_Next("Next"),
      m_Rewind("RW"),
      m_Forward("FF")
{
    set_title("Control Window");

    add(m_Table);
    m_Table.attach(m_Play, 0,2, 0,1);
    m_Table.attach(m_Pause, 2,3, 0,1);
    m_Table.attach(m_Stop, 3,4, 0,1);
    m_Table.attach(m_Prev, 0,1, 1,2);
    m_Table.attach(m_Next, 1,2, 1,2);
    m_Table.attach(m_Rewind, 2,3, 1,2);
    m_Table.attach(m_Forward, 3,4, 1,2);

    m_Play.signal_clicked().connect( sigc::mem_fun(*this, &ControlWindow::on_button_play) );
    m_Pause.signal_clicked().connect( sigc::mem_fun(*this, &ControlWindow::on_button_pause) );
    m_Stop.signal_clicked().connect( sigc::mem_fun(*this, &ControlWindow::on_button_stop) );
    m_Prev.signal_clicked().connect( sigc::mem_fun(*this, &ControlWindow::on_button_prev) );
    m_Next.signal_clicked().connect( sigc::mem_fun(*this, &ControlWindow::on_button_next) );
    m_Rewind.signal_clicked().connect( sigc::mem_fun(*this, &ControlWindow::on_button_rewind) );
    m_Forward.signal_clicked().connect( sigc::mem_fun(*this, &ControlWindow::on_button_forward) );

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
