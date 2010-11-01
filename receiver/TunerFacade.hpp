//
// Video 4 Linux Tuner Facade
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

#ifndef TUNER_CONFIGURATOR_HPP
#define TUNER_CONFIGURATOR_HPP

#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"
#include "receiver/ChannelFrequencyTable.hpp"
#include "receiver/GeneralEvents.hpp"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/string.hpp>

class Server;

struct TunerInit
{
    TunerInit(boost::shared_ptr<Server> server)
	: server(server)
    {}
    boost::shared_ptr<Server> server;
};

struct TunerOpen
{
    template<class Archive>
    void serialize(Archive&, const unsigned int)
    {}
};

struct TunerClose
{
    template<class Archive>
    void serialize(Archive&, const unsigned int)
    {}
};

struct TunerTuneChannel
{
    TunerTuneChannel(ChannelData channelData)
	: channelData(channelData)
    {}

    template<class Archive>
    void serialize(Archive& ar, const unsigned int)
    {
	ar & channelData;
    }

    ChannelData channelData;

    // private:
    TunerTuneChannel() {}
};

struct TunerCheckSignal {
};

struct TunerNotifyChannelTuned
{
    TunerNotifyChannelTuned(ChannelData channelData)
	: channelData(channelData)
    {}

    template<class Archive>
    void serialize(Archive& ar, const unsigned int)
    {
	ar & channelData;
    }

    ChannelData channelData;

    // private:
    TunerNotifyChannelTuned(){}
};

struct TunerNotifySignalDetected
{
    TunerNotifySignalDetected(ChannelData channelData)
	: channelData(channelData)
    {}

    template<class Archive>
    void serialize(Archive& ar, const unsigned int)
    {
	ar & channelData;
    }

    ChannelData channelData;

    // private:
    TunerNotifySignalDetected(){}
};

struct TunerStartScan
{
    TunerStartScan(std::string standard)
	: standard(standard)
    {}

    template<class Archive>
    void serialize(Archive& ar, const unsigned int)
    {
	ar & standard;
    }

    std::string standard;

    // private:
    TunerStartScan() {}
};

struct TunerScanStopped
{
    template<class Archive>
    void serialize(Archive&, const unsigned int)
    {}
};

struct TunerScanFinished
{
    template<class Archive>
    void serialize(Archive&, const unsigned int)
    {}
};

class TunerFacade : public event_receiver<TunerFacade>
{
    friend class event_processor<>;

public:
    TunerFacade(event_processor_ptr_type evt_proc)
	: base_type(evt_proc),
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
    void process(boost::shared_ptr<TunerInit> event);
    void process(boost::shared_ptr<TunerOpen> event);
    void process(boost::shared_ptr<TunerClose> event);

    void process(boost::shared_ptr<TunerTuneChannel> event);
    void process(boost::shared_ptr<TunerCheckSignal> event);

    void process(boost::shared_ptr<TunerStartScan> event);

    void detectSignal();
    void getFrequency();
    void setFrequency(const ChannelData& channelData);
    void setScanningFrequency();

    boost::shared_ptr<Server> server;

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

// ===================================================================
// Message Catalog additions for debugging:

inline std::ostream& operator<<(std::ostream& os, const TunerOpen&)
{
    return os << "TunerOpen()";
}

inline std::ostream& operator<<(std::ostream& os, const TunerClose&)
{
    return os << "TunerClose()";
}

inline std::ostream& operator<<(std::ostream& os, const TunerTuneChannel&)
{
    return os << "TunerTuneChannel()";
}

inline std::ostream& operator<<(std::ostream& os, const TunerStartScan&)
{
    return os << "TunerStartScan()";
}

inline std::ostream& operator<<(std::ostream& os, const TunerScanFinished&)
{
    return os << "TunerScanFinished()";
}
inline std::ostream& operator<<(std::ostream& os, const TunerScanStopped&)
{
    return os << "TunerScanStopped()";
}
inline std::ostream& operator<<(std::ostream& os, const TunerNotifySignalDetected&)
{
    return os << "TunerNotifySignalDetected()";
}
inline std::ostream& operator<<(std::ostream& os, const TunerNotifyChannelTuned&)
{
    return os << "TunerNotifyChannelTuned()";
}

#endif
