//
// Media Receiver Events
//
// Copyright (C) Joachim Erbs, 2010
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
