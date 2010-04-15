//
// Video 4 Linux Tuner Facade
//
// Copyright (C) Joachim Erbs, 2010
//

#ifndef TUNER_CONFIGURATOR_HPP
#define TUNER_CONFIGURATOR_HPP

#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"
#include "receiver/ChannelFrequencyTable.hpp"
#include "receiver/GeneralEvents.hpp"

struct TunerOpen {};
struct TunerClose {};

struct TunerTuneChannel
{
    TunerTuneChannel(ChannelData channelData)
	: channelData(channelData)
    {}
    ChannelData channelData;
};

struct TunerCheckSignal {};

struct TunerNotifyChannelTuned
{
    TunerNotifyChannelTuned(ChannelData channelData)
	: channelData(channelData)
    {}
    ChannelData channelData;
};

struct TunerNotifySignalDetected
{
    TunerNotifySignalDetected(ChannelData channelData)
	: channelData(channelData)
    {}
    ChannelData channelData;
};

struct TunerStartScan
{
    TunerStartScan(std::string standard)
	: standard(standard)
    {}
    std::string standard;
};

struct TunerScanStopped {};

struct TunerScanFinished {};


class TunerFacade : public event_receiver<TunerFacade>
{
    friend class event_processor<>;

public:
    TunerFacade(event_processor_ptr_type evt_proc)
	: base_type(evt_proc),
	  mediaReceiver(0),
	  fd(-1),
	  device("/dev/video0"),
	  signalDetected(false),
	  signalDetectionTime(getTimespec(0.1)),
	  retry(0),
	  numRetries(6),
	  scanning(false),
	  scanningChannel(0)
    {}

    ~TunerFacade()
    {}

private:
    void process(boost::shared_ptr<ReceiverInitEvent> event);
    void process(boost::shared_ptr<TunerOpen> event);
    void process(boost::shared_ptr<TunerClose> event);

    void process(boost::shared_ptr<TunerTuneChannel> event);
    void process(boost::shared_ptr<TunerCheckSignal> event);

    void process(boost::shared_ptr<TunerStartScan> event);

    void detectSignal();
    void getFrequency();
    void setFrequency(const ChannelData& channelData);
    void setScanningFrequency();

    MediaReceiver* mediaReceiver;

    int fd;
    char const * device;

    ChannelData tunedChannelData;
    bool signalDetected;

    timer signalDetectionTimer;
    timespec_t signalDetectionTime;
    int retry;
    int numRetries;

    bool scanning;
    ChannelFrequencyTable scanningChannelFrequencyTable;
    int scanningChannel;
    
};

#endif
