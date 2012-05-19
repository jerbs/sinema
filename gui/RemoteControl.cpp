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

#include "gui/RemoteControl.hpp"
#include "gui/KeyCodes.h"
#include "platform/Logging.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <linux/input.h>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <string.h>

// #undef TRACE_DEBUG
// #define TRACE_DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

RemoteControl::RemoteControl()
    : ir_receiver(0),
      fd(-1)
{
    get_input_device_list();
    guess_remote_control_device();
    open_input_device();
}

RemoteControl::~RemoteControl()
{
    close(fd);
}

bool RemoteControl::compare(RemoteControl::dev_input_event const & lh,
			    RemoteControl::dev_input_event const & rh)
{
    return (strverscmp(lh.device.c_str(), rh.device.c_str()) < 0) ? true : false;
}

void RemoteControl::get_input_device_list()
{
    TRACE_DEBUG();
    namespace fs = boost::filesystem;

    ir_receiver = 0;
    input_event_devices.clear();

    fs::path dev_input_dir("/dev/input");
    if (fs::is_directory(dev_input_dir))
    {
	fs::directory_iterator end_iter;
	for ( fs::directory_iterator dir_itr(dev_input_dir);
	      dir_itr != end_iter;
	      ++dir_itr )
	{
	    std::string path = dir_itr->path().string();
	    std::string filename = dir_itr->path().filename().string();
	    // TRACE_DEBUG(<< path << ", " << filename);
	    if (filename.compare(0,5,"event") == 0)
	    {
		// TRACE_DEBUG(<< "ok");
		int fd = open(path.c_str(), O_RDONLY);
		if (fd < 0)
		{
		    continue;
		}

		char name[256] = "???";
		ioctl(fd, EVIOCGNAME(sizeof(name)), name);

		dev_input_event elem;
		elem.device = path;
		elem.name = std::string(name);

		TRACE_DEBUG(<< path << ": " << name);

		input_event_devices.push_back(elem);
	    }
	}

	sort(input_event_devices.begin(),
	     input_event_devices.end(),
	     &RemoteControl::compare);
    }
}

void RemoteControl::guess_remote_control_device()
{
    int high_Score = 0;
    dev_input_event* best = 0;
    for (dev_input_event & entry : input_event_devices)
    {
	TRACE_DEBUG(<< entry.device << ": " << entry.name);
	static const boost::regex irRegExp("^.*\\bir\\b.*$",
					   boost::regex_constants::perl |
					   boost::regex_constants::icase);
	static const boost::regex receiverRegExp("^.*\\breceiver\\b.*$",
						 boost::regex_constants::perl |
						 boost::regex_constants::icase);
	int score = 0;
	if (regex_match(entry.name, irRegExp))
	{
	    score += 10;
	}
	if (regex_match(entry.name, receiverRegExp))
	{
	    score += 1;
	}
	TRACE_DEBUG(<< "score = " << score );

	if (score > high_Score)
	{
	    high_Score = score;
	    ir_receiver = &entry;
	}
    }
}

void RemoteControl::open_input_device()
{
    if (! ir_receiver)
	return;

    TRACE_DEBUG(<< "opening " << ir_receiver->device);

    fd = open(ir_receiver->device.c_str(), O_RDONLY);
    if (fd == -1)
    {
	TRACE_ERROR("open failed: " << strerror(errno));
    }

    Glib::RefPtr<Glib::MainContext> mainContext = Glib::MainContext::get_default();
    int priority = 1;
    Glib::PollFD pollFD(fd);
    mainContext->add_poll(pollFD, priority);
    Glib::IOCondition condition = Glib::IO_IN | Glib::IO_ERR;
    Glib::signal_io().connect(sigc::mem_fun(*this, &RemoteControl::on_readable), fd, condition);
}

bool RemoteControl::on_readable(Glib::IOCondition)
{
    TRACE_DEBUG("RemoteControl::on_readable");

    input_event evl[64];

    ssize_t rd = read(fd, &evl, sizeof(evl));

    if (rd < (ssize_t)sizeof(input_event))
    {
	TRACE_ERROR("read failed: " << strerror(errno));
    }
    else
    {
	int cnt = rd / sizeof(input_event);
	for (int i = 0; i < cnt; i++)
	{


	    if (evl[i].type == EV_SYN)
	    {
		TRACE_DEBUG(<< "Sync (" << evl[i].code << "," << evl[i].value << ")");
	    }
	    else if (evl[i].type == EV_KEY)
	    {
		const char* what;
		switch (evl[i].value)
		{
		case 0: what = "released"; break;
		case 1: what = "pressed"; break;
		default: what = "?";
		}
		TRACE_DEBUG(<< "Key (" << get_key_description(evl[i].code) << ")" << what);
		on_key_pressed(evl[i].code);
	    }
	    else
	    {
		TRACE_DEBUG(<< "type=" << evl[i].type
			    << ", code=" << evl[i].code
			    << ", value=" << evl[i].value);
	    }
	}
    }

    return true;
}

void RemoteControl::on_key_pressed(uint16_t code)
{
    switch(code)
    {
    case KEY_PLAY:
	signal_play();
	break;
    case KEY_PAUSE:
	signal_pause();
	break;
    case KEY_STOP:
	signal_stop();
	break;
    case KEY_RECORD:
	signal_record();
	break;
    case KEY_REWIND:
	signal_rewind();
	break;
    case KEY_FORWARD:
	signal_forward();
	break;
    case KEY_INFO:
	signal_info();
	break;
    case KEY_EPG:
	signal_epg();
	break;

    case KEY_OK:
	signal_ok();
	break;
    case KEY_UP:
	signal_up();
	break;
    case KEY_DOWN:
	signal_down();
	break;
    case KEY_LEFT:
	signal_left();
	break;
    case KEY_RIGHT:
	signal_right();
	break;

    case KEY_VOLUMEUP:
	signal_volumeup(3);
	break;
    case KEY_VOLUMEDOWN:
	signal_volumedown(3);
	break;

    case KEY_TEXT:
	signal_text();
	break;
    case KEY_CHANNELUP:
	signal_channelup();
	break;
    case KEY_CHANNELDOWN:
	signal_channeldown();
	break;
    case KEY_EXIT:
	signal_exit();
	break;

    case KEY_RED:
	signal_red();
	break;
    case KEY_GREEN:
	signal_green();
	break;
    case KEY_YELLOW:
	signal_yellow();
	break;
    case KEY_BLUE:
	signal_blue();
	break;

    case KEY_MUTE:
	signal_mute();
	break;
    case KEY_POWER:
	signal_power();
	break;
    case KEY_SHUFFLE:
	signal_shuffle();
	break;
    case KEY_MODE:
	signal_mode();
	break;

    case KEY_1:
	signal_1();
	break;
    case KEY_2:
	signal_2();
	break;
    case KEY_3:
	signal_3();
	break;
    case KEY_4:
	signal_4();
	break;
    case KEY_5:
	signal_5();
	break;
    case KEY_6:
	signal_6();
	break;
    case KEY_7:
	signal_7();
	break;
    case KEY_8:
	signal_8();
	break;
    case KEY_9:
	signal_9();
	break;
    case KEY_0:
	signal_0();
	break;

    default:
	break;
    }
}
