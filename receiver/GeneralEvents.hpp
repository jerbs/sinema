//
// Media Receiver Events
//
// Copyright (C) Joachim Erbs, 2010
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

#ifndef RECEIVER_GENERAL_EVENTS_HPP
#define RECEIVER_GENERAL_EVENTS_HPP

#include <boost/shared_ptr.hpp>
#include <string>

class MediaReceiver;
class TunerFacade;

struct ReceiverInitEvent
{
    MediaReceiver* mediaReceiver;
    boost::shared_ptr<TunerFacade> tunerFacade;
};

struct ChannelData
{
    ChannelData()
	: frequency(0),
	  finetune(0)
    {}

    std::string standard;
    std::string channel;
    int frequency;
    int finetune;
    int getTunedFrequency() const {return frequency+finetune;}
};

#endif
