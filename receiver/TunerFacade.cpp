//
// Video 4 Linux Tuner Facade
//
// Copyright (C) Joachim Erbs, 2010
//

#include "receiver/TunerFacade.hpp"
#include "receiver/MediaReceiver.hpp"

#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

void TunerFacade::process(boost::shared_ptr<ReceiverInitEvent> event)
{
    mediaReceiver = event->mediaReceiver;
}

void TunerFacade::process(boost::shared_ptr<TunerOpen> event)
{
    if ((fd = open(device, O_RDWR)) < 0)
    {
        ERROR( << "Failed to open " << device << ": " << strerror(errno) );
    }
    else
    {
	getFrequency();
    }
}

void TunerFacade::process(boost::shared_ptr<TunerClose> event)
{
    close(fd);
    fd = -1;
}

void TunerFacade::process(boost::shared_ptr<TunerTuneChannel> event)
{
    if (fd < 0)
	return;

    if (scanning)
    {
	mediaReceiver->queue_event(boost::make_shared<TunerScanStopped>());
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
        ERROR( << "ioctl VIDIOC_G_FREQUENCY failed: " << strerror(errno) );
	return;
    }

    tunedChannelData.frequency = (vf.frequency * 1000) / 16;
    mediaReceiver->queue_event(boost::make_shared<TunerNotifyChannelTuned>(tunedChannelData));
    detectSignal();
}

void TunerFacade::setFrequency(const ChannelData& channelData)
{
    struct v4l2_frequency vf;
    vf.tuner = 0;
    vf.type = V4L2_TUNER_ANALOG_TV;
    vf.frequency = (channelData.getTunedFrequency() * 16)/1000;
    memset(vf.reserved, 0, sizeof(vf.reserved));

    int result = ioctl(fd, VIDIOC_S_FREQUENCY, &vf);

    if (result < 0)
    {
        ERROR( << "ioctl VIDIOC_S_FREQUENCY failed: " << strerror(errno) );
	return;
    }

    tunedChannelData = channelData;
    mediaReceiver->queue_event(boost::make_shared<TunerNotifyChannelTuned>(channelData));
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
	ERROR( << "ioctl VIDIOC_S_FREQUENCY failed" << strerror(errno) );
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
	DEBUG( << "signal detected: retry = " << retry );
	mediaReceiver->queue_event(boost::make_shared<TunerNotifySignalDetected>(tunedChannelData));
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
	mediaReceiver->queue_event(boost::make_shared<TunerScanFinished>());
    }
}
