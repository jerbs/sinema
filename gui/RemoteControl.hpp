//
// Remote Control
//
// Copyright (C) Joachim Erbs, 2012
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

#ifndef REMOTE_CONTROL_HPP
#define REMOTE_CONTROL_HPP

#include <glibmm/main.h>
#include <string>
#include <vector>

class RemoteControl
{
public:
    RemoteControl();
    ~RemoteControl();

    sigc::signal<void> signal_play;
    sigc::signal<void> signal_pause;
    sigc::signal<void> signal_stop;
    sigc::signal<void> signal_record;
    sigc::signal<void> signal_rewind;
    sigc::signal<void> signal_forward;
    sigc::signal<void> signal_info;
    sigc::signal<void> signal_epg;
    sigc::signal<void> signal_ok;
    sigc::signal<void> signal_up;
    sigc::signal<void> signal_down;
    sigc::signal<void> signal_left;
    sigc::signal<void> signal_right;
    sigc::signal<void,int> signal_volumeup;
    sigc::signal<void,int> signal_volumedown;
    sigc::signal<void> signal_text;
    sigc::signal<void> signal_channelup;
    sigc::signal<void> signal_channeldown;
    sigc::signal<void> signal_exit;
    sigc::signal<void> signal_red;
    sigc::signal<void> signal_green;
    sigc::signal<void> signal_yellow;
    sigc::signal<void> signal_blue;
    sigc::signal<void> signal_mute;
    sigc::signal<void> signal_power;
    sigc::signal<void> signal_shuffle;
    sigc::signal<void> signal_mode;
    sigc::signal<void> signal_1;
    sigc::signal<void> signal_2;
    sigc::signal<void> signal_3;
    sigc::signal<void> signal_4;
    sigc::signal<void> signal_5;
    sigc::signal<void> signal_6;
    sigc::signal<void> signal_7;
    sigc::signal<void> signal_8;
    sigc::signal<void> signal_9;
    sigc::signal<void> signal_0;

private:
    struct dev_input_event
    {
	std::string device;
	std::string name;
    };

    std::vector<dev_input_event> input_event_devices;
    dev_input_event* ir_receiver;

    void get_input_device_list();
    void guess_remote_control_device();
    bool open_input_device();

    bool on_readable(Glib::IOCondition);
    void on_key_pressed(uint16_t code);
    static bool compare(dev_input_event const & lh, dev_input_event const & rh);

    int fd;
    Glib::PollFD m_PollFd;
};

#endif
