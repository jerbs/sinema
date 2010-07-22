//
// Video Output
//
// Copyright (C) Joachim Erbs, 2009-2010
//

#include "player/VideoOutput.hpp"
#include "player/VideoDecoder.hpp"
#include "player/MediaPlayer.hpp"
#include "player/Demuxer.hpp"
#include "player/XlibFacade.hpp"

#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <sys/types.h>

using namespace std;

int tidVideoOutput = 0;

void VideoOutput::process(boost::shared_ptr<InitEvent> event)
{
    if (state == IDLE)
    {
	TRACE_DEBUG(<< "tid = " << gettid());

	tidVideoOutput = gettid();

	mediaPlayer = event->mediaPlayer;
	demuxer = event->demuxer;
	videoDecoder = event->videoDecoder;

	state = INIT;
    }
}

void VideoOutput::process(boost::shared_ptr<OpenVideoOutputReq> event)
{
    if (state == INIT)
    {
	TRACE_DEBUG();

	xfVideo->resize(event->width, event->height, event->parNum, event->parDen, event->imageFormat);
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
	    frameQueue.pop();
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
    // VideoDecoder sends this event for every XFVideoImage received with wrong size.
    // This case occurs when:
    // 1) Starting to play a video with a differnt resolution and the VideoDecoder 
    //    gets the XFVideoImage created in VideoOutput::showBlackFrame while closing
    //    the previous video.
    // 2) The video resolution changes within one video.

    if (isOpen())
    {
	TRACE_DEBUG();

	xfVideo->resize(event->width, event->height, event->parNum, event->parDen, event->imageFormat);
	createVideoImage();
    }
}

void VideoOutput::process(boost::shared_ptr<XFVideoImage> event)
{
    if (isOpen())
    {
	TRACE_DEBUG();

	eos = false;
	frameQueue.push(event);

	switch (state)
	{
	case OPEN:
	    state = STILL;
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

void VideoOutput::process(boost::shared_ptr<DeleteXFVideoImage>)
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

	    TRACE_DEBUG(<< "audioSnapshotPTS =" << audioSnapshotPTS
			<< ", audioSnapshotTime=" << audioSnapshotTime);

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
	    boost::shared_ptr<XFVideoImage> image(frameQueue.front());
	    frameQueue.pop();
	    videoDecoder->queue_event(image);
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
	if (frameQueue.empty())
	{
	    mediaPlayer->queue_event(boost::make_shared<EndOfVideoStream>());
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

	xfVideo = boost::make_shared<XFVideo>(static_cast<Display*>(event->display),
					      event->window, 720, 576, fct, fct2);

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

void VideoOutput::process(boost::shared_ptr<CommandPlay>)
{
    if (isOpen())
    {
	TRACE_DEBUG();

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
    videoDecoder->queue_event(boost::make_shared<XFVideoImage>(xfVideo));
}

void VideoOutput::displayNextFrame()
{
    if (frameQueue.empty())
    {
	// No frame available to display.
	state = STILL;
	if (eos)
	{
	    mediaPlayer->queue_event(boost::make_shared<EndOfVideoStream>());
	}
	return;
    }

    TRACE_DEBUG();

    boost::shared_ptr<XFVideoImage> image(frameQueue.front());
    frameQueue.pop();

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

    boost::shared_ptr<XFVideoImage> previousImage = xfVideo->show(image);

    int currentTime = image->getPTS();
    if (currentTime != lastNotifiedTime)
    {
	mediaPlayer->queue_event(boost::make_shared<NotificationCurrentTime>(currentTime));
	lastNotifiedTime = currentTime;
    }
   
    if (previousImage)
    {
	videoDecoder->queue_event(previousImage);
    }

    startFrameTimer();
}

void VideoOutput::startFrameTimer()
{
    if (frameQueue.empty() || !audioSync)
    {
	if ( !frameQueue.empty() &&
	     videoStreamOnly )
	{
	    // No audio stream available, simulate audioSync
	    audioSync = true;
	    audioSnapshotPTS = displayedFramePTS;
	    audioSnapshotTime = frameTimer.get_current_time();
	}
	else
	{
	    // To calculate the timer the next frame and audio synchronization 
	    // information must be available.
	    state = STILL;
	    if (eos)
	    {
		mediaPlayer->queue_event(boost::make_shared<EndOfVideoStream>());
	    }
	    return;
	}
    }

    TRACE_DEBUG();

    boost::shared_ptr<XFVideoImage> nextFrame(frameQueue.front());

    timespec_t currentTime = frameTimer.get_current_time();
    timespec_t audioDeltaTime = currentTime - audioSnapshotTime;
    double currentPTS = audioSnapshotPTS + getSeconds(audioDeltaTime);
    double nextFrameVideoPTS = nextFrame->getPTS();
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
    boost::shared_ptr<XFVideoImage> yuvImage(new XFVideoImage(xfVideo));
    yuvImage->createBlackImage();
    // yuvImage->createPatternImage();
    // yuvImage->createDemoImage();
    xfVideo->show(yuvImage);
}

void VideoOutput::sendNotificationVideoSize(boost::shared_ptr<NotificationVideoSize> event)
{
    mediaPlayer->queue_event(event);
}

void VideoOutput::sendNotificationClipping(boost::shared_ptr<NotificationClipping> event)
{
    mediaPlayer->queue_event(event);
}
