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

#include "receiver/TunerFacade.hpp"
#include "daemon/Server.hpp"

#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

void TunerFacade::process(boost::shared_ptr<TunerInit> event)
{
    server = event->server;
}

void TunerFacade::process(boost::shared_ptr<TunerOpen>)
{
    if ((fd = open(device, O_RDWR)) < 0)
    {
        TRACE_ERROR( << "Failed to open " << device << ": " << strerror(errno) );
    }
    else
    {
	getFrequency();
    }
}

void TunerFacade::process(boost::shared_ptr<TunerClose>)
{
    close(fd);
    fd = -1;
}

void TunerFacade::process(boost::shared_ptr<TunerTuneChannel> event)
{
    TRACE_DEBUG(<< event->channelData.getTunedFrequency());

    if (fd < 0)
	return;

    if (scanning)
    {
	server->queue_event(boost::make_shared<TunerScanStopped>());
    }

    scanning = false;

    setFrequency(event->channelData);
}

void TunerFacade::getFrequency()
{
    struct v4l2_frequency vf;
    vf.tuner = 0;
    vf.type = V4L2_TUNER_ANALOG_TV;
    vf.frequency = 0;
    memset(vf.reserved, 0, sizeof(vf.reserved));

    int result = ioctl(fd, VIDIOC_G_FREQUENCY, &vf);
    if (result < 0)
    {
        TRACE_ERROR( << "ioctl VIDIOC_G_FREQUENCY failed: " << strerror(errno) );
	return;
    }

    tunedChannelData.frequency = (vf.frequency * 1000) / 16;
    server->queue_event(boost::make_shared<TunerNotifyChannelTuned>(tunedChannelData));
    detectSignal();
}

void TunerFacade::setFrequency(const ChannelData& channelData)
{
    struct v4l2_frequency vf;
    vf.tuner = 0;
    vf.type = V4L2_TUNER_ANALOG_TV;
    vf.frequency = (channelData.getTunedFrequency() * 16)/1000;
    memset(vf.reserved, 0, sizeof(vf.reserved));

    TRACE_DEBUG(<< vf.frequency);

    int result = ioctl(fd, VIDIOC_S_FREQUENCY, &vf);

    if (result < 0)
    {
        TRACE_ERROR( << "ioctl VIDIOC_S_FREQUENCY failed: " << strerror(errno) );
	return;
    }

    tunedChannelData = channelData;
    server->queue_event(boost::make_shared<TunerNotifyChannelTuned>(channelData));
    detectSignal();
}

void TunerFacade::detectSignal()
{
    signalDetected = false;

    signalDetectionTimer.relative(signalDetectionTime);
    retry = 0;
    start_timer(boost::make_shared<TunerCheckSignal>(),
		signalDetectionTimer);
}

void TunerFacade::process(boost::shared_ptr<TunerCheckSignal> event)
{
    if (fd < 0)
	return;

    struct v4l2_tuner vt;
    memset(&vt, 0, sizeof(vt));
    vt.index = 0;

    int result = ioctl(fd, VIDIOC_G_TUNER, &vt);

    if (result < 0)
    {
	TRACE_ERROR( << "ioctl VIDIOC_S_FREQUENCY failed" << strerror(errno) );
	return;
    }

    if (!vt.signal)
    {
	// No Signal detected yet.

	if (++retry < numRetries)
	{
	    // Wait.
	    signalDetectionTimer.relative(signalDetectionTime);
	    start_timer(event, signalDetectionTimer);
	    return;
	}
	else
	{
	    // Give up.
	}
    }
    else
    {
	// Signal detected.
	signalDetected = true;
	TRACE_DEBUG( << "signal detected: retry = " << retry );
	server->queue_event(boost::make_shared<TunerNotifySignalDetected>(tunedChannelData));
    }

    // Has given up for this channel OR signal detected.

    if (scanning)
    {
	scanningChannel++;
	setScanningFrequency();
    }
}

void TunerFacade::process(boost::shared_ptr<TunerStartScan> event)
{
    if (fd < 0)
	return;

    scanning = true;
    scanningChannelFrequencyTable = ChannelFrequencyTable::create(event->standard.c_str());
    scanningChannel = 0;
    setScanningFrequency();
}

void TunerFacade::setScanningFrequency()
{
    int frequency = ChannelFrequencyTable::getChannelFreq(scanningChannelFrequencyTable, scanningChannel);

    if (frequency)
    {
	ChannelData channelData;
	channelData.standard = std::string(scanningChannelFrequencyTable.getStandard_cstr());
	channelData.channel = std::string(scanningChannelFrequencyTable.getChannelName_cstr(scanningChannel));
	channelData.frequency = frequency;
	channelData.finetune = 0;

	setFrequency(channelData);
    }
    else
    {
	// Reached end of channel table.
	scanning = false;
	scanningChannel = 0;
	server->queue_event(boost::make_shared<TunerScanFinished>());
    }
}
