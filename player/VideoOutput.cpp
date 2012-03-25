//
// Video Output
//
// Copyright (C) Joachim Erbs, 2009-2010
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

#include "player/VideoOutput.hpp"
#include "player/VideoDecoder.hpp"
#include "player/Deinterlacer.hpp"
#include "player/MediaPlayer.hpp"
#include "player/Demuxer.hpp"
#include "player/XlibFacade.hpp"

#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <sys/types.h>

using namespace std;

int tidVideoOutput = 0;

VideoOutput::VideoOutput(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      eos(false),
      state(IDLE),
      audioSync(false),
      audioSnapshotPTS(0),
      ignoreAudioSync(0),
      videoStreamOnly(false),
      lastNotifiedTime(-1),
      displayedFramePTS(0)
{
    TRACE_DEBUG(<< "tid = " << gettid());

    audioSnapshotTime.tv_sec  = 0; 
    audioSnapshotTime.tv_nsec = 0;
}

VideoOutput::~VideoOutput()
{
    TRACE_DEBUG(<< "tid = " << gettid());
}

void VideoOutput::process(boost::shared_ptr<InitEvent> event)
{
    if (state == IDLE)
    {
	TRACE_DEBUG(<< "tid = " << gettid());

	tidVideoOutput = gettid();

	mediaPlayer = event->mediaPlayer;
	demuxer = event->demuxer;
	videoDecoder = event->videoDecoder;
	deinterlacer = event->deinterlacer;

	state = INIT;
    }
}

void VideoOutput::process(boost::shared_ptr<OpenVideoOutputReq> event)
{
    if (state == INIT)
    {
	TRACE_DEBUG();

	xfVideo->resize(event->width, event->height, event->parNum, event->parDen, event->fourccFormat);
	for (int i=0; i<10; i++)
	{
	    createVideoImage();
	}

	state = OPEN;

	videoDecoder->queue_event(boost::make_shared<OpenVideoOutputResp>());
    }
}

void VideoOutput::process(boost::shared_ptr<CloseVideoOutputReq>)
{
    if (isOpen())
    {
	TRACE_DEBUG();

	showBlackFrame();

	// Throw away all queued frames:
	while ( !frameQueue.empty() )
	{
	    frameQueue.pop_front();
	}

	state = INIT;
	audioSync = false;
	lastNotifiedTime = -1;
	displayedFramePTS = 0;
	ignoreAudioSync = 0;

	audioSyncInfo.reset();

	videoStreamOnly = false;

	videoDecoder->queue_event(boost::make_shared<CloseVideoOutputResp>());
    }
}

void VideoOutput::process(boost::shared_ptr<ResizeVideoOutputReq> event)
{
    // VideoDecoder sends this event for every XFVideoImage received with wrong size or image format.
    // This case occurs when:
    // 1) Starting to play a video with a different resolution and the VideoDecoder 
    //    gets the XFVideoImage created in VideoOutput::showBlackFrame while closing
    //    the previous video.
    // 2) The video resolution changes within one video.

    if (isOpen())
    {
	TRACE_DEBUG();

	xfVideo->resize(event->width, event->height, event->parNum, event->parDen, event->fourccFormat);
	createVideoImage();
    }
}

void VideoOutput::process(std::unique_ptr<XFVideoImage> event)
{
    if (isOpen())
    {
	TRACE_DEBUG();

	eos = false;
	firstFrame = false;
	frameQueue.push_back(std::move(event));

	switch (state)
	{
	case OPEN:
	    state = STILL;
	    firstFrame = true;
	    displayNextFrame();
	    break;

	case STILL:
	    startFrameTimer();
	    break;

	case FLUSHED:
	    state = STILL;
	    displayNextFrame();
	    break;

	default:
	    break;
	}
    }
}

void VideoOutput::process(std::unique_ptr<DeleteXFVideoImage>)
{
    // This class is running in the GUI thread. Here it is safe to delete
    // X11 resources. The XvImage contained in the XFVideoImage object
    // is deleted by not storing the shared_ptr.

    TRACE_DEBUG(<< "tid = " << gettid());
}

void VideoOutput::process(boost::shared_ptr<ShowNextFrame>)
{
    if (state == PLAYING)
    {
	TRACE_DEBUG();

	displayNextFrame();
    }
}

void VideoOutput::process(boost::shared_ptr<AudioSyncInfo> event)
{
    audioSyncInfo.reset();
    videoStreamOnly = false;

    if (isOpen())
    {
	if (ignoreAudioSync == 0)
	{
	    audioSync = true;
	    audioSnapshotPTS = event->pts;
	    audioSnapshotTime = event->abstime;

	    TRACE_DEBUG(<< "audioSnapshotPTS=" << audioSnapshotPTS
			<< ", audioSnapshotTime=" << audioSnapshotTime
			<< ", state=" << state);

	    if (state == STILL)
	    {
		startFrameTimer();
	    }
	}

	if (ignoreAudioSync == -1)
	{
	    // AudioFlushedInd received, but FlushReq is not yet received.
	    // Defer AudioSyncInfo event:
	    audioSyncInfo = event;
	}
    }
}

void VideoOutput::process(boost::shared_ptr<FlushReq>)
{
    if (isOpen())
    {
	TRACE_DEBUG();

	// Send received frames back to VideoDecoder without showing them:
	while (!frameQueue.empty())
	{
	    std::unique_ptr<XFVideoImage> image(std::move(frameQueue.front()));
	    frameQueue.pop_front();
	    videoDecoder->queue_event(std::move(image));
	}

	// Timeout event ShowNextFrame may be received after the timer is
	// stopped. In this case the frameQueue is empty and the event will
	// be ignored.
	stop_timer(frameTimer);

	// No frame available to display. In state FLUSHED the next frame is
	// immediately shown when received without starting a timer before,
	// as it would be done in state STILL.
	state = FLUSHED;

	// Audio synchronization has to be reestablished:
	audioSync = false;

	ignoreAudioSync++;

	if (audioSyncInfo)
	{
	    // Process deferred AudioSyncInfo event:
	    process(audioSyncInfo);
	}
    }
}

void VideoOutput::process(boost::shared_ptr<AudioFlushedInd>)
{
    if (isOpen())
    {
	TRACE_DEBUG();
	ignoreAudioSync--;
    }
}

void VideoOutput::process(boost::shared_ptr<SeekRelativeReq> event)
{
    event->displayedFramePTS = displayedFramePTS;
    demuxer->queue_event(event);
}

void VideoOutput::process(boost::shared_ptr<EndOfVideoStream>)
{
    if (isOpen())
    {
	TRACE_DEBUG();
	eos = true;

	if (state == STILL)
	{
	    startFrameTimer();
	}
    }
}

void VideoOutput::process(boost::shared_ptr<NoAudioStream>)
{
    videoStreamOnly = true;
    if (state == STILL)
    {
	startFrameTimer();
    }
}

void VideoOutput::process(boost::shared_ptr<WindowRealizeEvent> event)
{
    if (!xfVideo)
    {
	TRACE_DEBUG();

	// Make VideoOutput::sendNotificationVideoSize accessable for XFVideo: 
	typedef void (VideoOutput::*fct_t)(boost::shared_ptr<NotificationVideoSize>);
	fct_t tmp = &VideoOutput::sendNotificationVideoSize;
	boost::function<void (boost::shared_ptr<NotificationVideoSize>)> fct = boost::bind(tmp, this, _1);

	typedef void (VideoOutput::*fct2_t)(boost::shared_ptr<NotificationClipping>);
	fct2_t tmp2 = &VideoOutput::sendNotificationClipping;
	boost::function<void (boost::shared_ptr<NotificationClipping>)> fct2 = boost::bind(tmp2, this, _1);

	typedef void (VideoOutput::*fct3_t)(boost::shared_ptr<NotificationVideoAttribute>);
	fct3_t tmp3 = &VideoOutput::sendNotificationVideoAttribute;
	boost::function<void (boost::shared_ptr<NotificationVideoAttribute>)> fct3 = boost::bind(tmp3, this, _1);

	xfVideo = boost::make_shared<XFVideo>(static_cast<Display*>(event->display),
					      event->window, 720, 576, fct, fct2, fct3,
					      event->addWindowSystemEventFilter);

	showBlackFrame();
    }
}

void VideoOutput::process(boost::shared_ptr<WindowConfigureEvent> event)
{
    TRACE_DEBUG();
    if (xfVideo)
    {
	xfVideo->handleConfigureEvent(event);
	if (!isOpen())
	{
	    showBlackFrame();
	}
    }
}

void VideoOutput::process(boost::shared_ptr<WindowExposeEvent>)
{
    TRACE_DEBUG();
    if (xfVideo)
    {
	xfVideo->handleExposeEvent();
	if (!isOpen())
	{
	    showBlackFrame();
	}
    }
}

void VideoOutput::process(boost::shared_ptr<ClipVideoDstEvent> event)
{
    if (xfVideo)
    {
	xfVideo->clipDst(event->left, event->right, event->top, event->bottom);
    }
}

void VideoOutput::process(boost::shared_ptr<ClipVideoSrcEvent> event)
{
    if (xfVideo)
    {
	xfVideo->clipSrc(event->left, event->right, event->top, event->bottom);
    }
}

void VideoOutput::process(boost::shared_ptr<EnableXvClipping>)
{
    if (xfVideo)
    {
	xfVideo->enableXvClipping();
    }
}

void VideoOutput::process(boost::shared_ptr<DisableXvClipping>)
{
    if (xfVideo)
    {
	xfVideo->disableXvClipping();
    }
}

void VideoOutput::process(boost::shared_ptr<ChangeVideoAttribute> event)
{
    if (xfVideo)
    {
	xfVideo->setXvPortAttributes(event->name, event->value);
    }
}

void VideoOutput::process(boost::shared_ptr<CommandPlay>)
{
    if (isOpen())
    {
	TRACE_DEBUG();

	state = STILL;
	displayNextFrame();
    }
}

void VideoOutput::process(boost::shared_ptr<CommandPause>)
{
    if (isOpen())
    {
	TRACE_DEBUG();

	state = PAUSE;
	audioSync = false;
    }
}

void VideoOutput::createVideoImage()
{
    videoDecoder->queue_event(std::unique_ptr<XFVideoImage>(new XFVideoImage(xfVideo)));
}

void VideoOutput::displayNextFrame()
{
    if (!frameQueue.empty())
    {
	TRACE_DEBUG();

	std::unique_ptr<XFVideoImage> image(std::move(frameQueue.front()));
	frameQueue.pop_front();

	displayedFramePTS = image->getPTS();

	{
	    // For debugging only:
	    timespec_t currentTime = frameTimer.get_current_time();
	    timespec_t audioDeltaTime = currentTime - audioSnapshotTime;
	    double currentAudioPTS = audioSnapshotPTS + getSeconds(audioDeltaTime);

#ifdef SYNCTEST
	    std::cout << "displayedFramePTS=" << displayedFramePTS << std::endl;
#endif

	    TRACE_INFO(<< "VOUT: currentTime=" <<  currentTime
		       << ", displayedFramePTS=" << displayedFramePTS
		       << ", currentAudioPTS=" << currentAudioPTS
		       << ", AVoffsetPTS=" << displayedFramePTS-currentAudioPTS
		       << ", audioDeltaTime=" << audioDeltaTime);
	}

	int currentTime = image->getPTS();
	if (currentTime != lastNotifiedTime)
	{
	    mediaPlayer->queue_event(boost::make_shared<NotificationCurrentTime>(currentTime));
	    lastNotifiedTime = currentTime;
	}

	std::unique_ptr<XFVideoImage> previousImage = std::move(xfVideo->show(std::move(image)));
	if (previousImage)
	{
	    videoDecoder->queue_event(std::move(previousImage));
	}
    }

    startFrameTimer();
}

void VideoOutput::startFrameTimer()
{
    if (frameQueue.empty())
    {
	// To calculate the timer the next frame and audio synchronization 
	// information must be available.

	state = STILL;

	if (eos && firstFrame && (videoStreamOnly || !audioSync))
	{
	    // Maybe it is a single photo without audio. Do not yet send an EndOfVideoStream 
	    // event. This will display the photo until the user skips to another file.

	    // Note: The AudioSyncInfo event may not have been received here.

	    return;
	}

	if (eos)
	{
	    // Sending EndOfVideoStream will close the file when audio playback has finished.

	    // Minor issue: If it is a video only stream, then the file is immediately closed.
	    // I.e. the last frame is displayed for a very short time only.

	    TRACE_DEBUG(<< "sending EndOfVideoStream");
	    mediaPlayer->queue_event(boost::make_shared<EndOfVideoStream>());
	}
	return;
    }

    if (videoStreamOnly)
    {
	// No audio stream available

	if (!audioSync)
	{
	    audioSync = true;
	    audioSnapshotPTS = displayedFramePTS;
	    audioSnapshotTime = frameTimer.get_current_time();
	}
    }
    else if (!audioSync)
    {
	// Wait for AudioSyncInfo event or NoAudioStream event.
	return;
    }

    TRACE_DEBUG();

    std::list<std::unique_ptr<XFVideoImage> >::iterator itNextFrame = frameQueue.begin();

    timespec_t currentTime = frameTimer.get_current_time();
    timespec_t audioDeltaTime = currentTime - audioSnapshotTime;
    double currentPTS = audioSnapshotPTS + getSeconds(audioDeltaTime);
    double nextFrameVideoPTS = (*itNextFrame)->getPTS();
    double videoDeltaPTS = nextFrameVideoPTS - currentPTS;

    if (videoDeltaPTS > 0)
    {
	timespec_t videoDeltaTime = getTimespec(videoDeltaPTS);

#if 1
	TRACE_INFO( << "VOUT: startFrameTimer: currentTime=" << currentTime
		    << ", currentPTS=" << currentPTS
		    << ", nextFrameVideoPTS=" << nextFrameVideoPTS
		    << ", waitTime=" << videoDeltaTime << videoDeltaPTS );
#endif

	frameTimer.relative(videoDeltaTime);
	start_timer(boost::make_shared<ShowNextFrame>(), frameTimer);
    }
    else
    {
	queue_event(boost::make_shared<ShowNextFrame>());
    }

    state = PLAYING;
}

void VideoOutput::showBlackFrame()
{
    std::unique_ptr<XFVideoImage> yuvImage(new XFVideoImage(xfVideo));
    yuvImage->createBlackImage();
    // yuvImage->createPatternImage();
    // yuvImage->createDemoImage();

    // Above a new XFVideoImage object was created. Now the XFVideoImage object returned by
    // the XFVideo::show method is thrown away. This keeps the number of created XFVideoImage
    // objects at a constant level.
    xfVideo->show(std::move(yuvImage));
}

void VideoOutput::sendNotificationVideoSize(boost::shared_ptr<NotificationVideoSize> event)
{
    mediaPlayer->queue_event(event);
}

void VideoOutput::sendNotificationClipping(boost::shared_ptr<NotificationClipping> event)
{
    mediaPlayer->queue_event(event);
    deinterlacer->queue_event(event);
}
void VideoOutput::sendNotificationVideoAttribute(boost::shared_ptr<NotificationVideoAttribute> event)
{
    if (state == PAUSE)
    {
	// Get the current image displayed with the new video attributes.
	// This is especially needed when changing Xv attributes from another 
	// application, e.g. gxvattr.
	xfVideo->handleExposeEvent();
    }

    mediaPlayer->queue_event(event);
}
