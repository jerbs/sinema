//
// Video Output
//
// Copyright (C) Joachim Erbs, 2009
//

#include "player/VideoOutput.hpp"
#include "player/VideoDecoder.hpp"
#include "player/MediaPlayer.hpp"
#include "player/XlibFacade.hpp"

#ifdef SYNCTEST
#include "SyncTest.hpp"
#endif

#include <boost/make_shared.hpp>
#include <boost/bind.hpp>

using namespace std;

void VideoOutput::process(boost::shared_ptr<InitEvent> event)
{
    if (state == IDLE)
    {
	DEBUG();

	mediaPlayer = event->mediaPlayer;
	demuxer = event->demuxer;
#ifdef SYNCTEST
	syncTest = event->syncTest;
#else
	videoDecoder = event->videoDecoder;
#endif
	state = INIT;
    }
}

void VideoOutput::process(boost::shared_ptr<OpenVideoOutputReq> event)
{
    if (state == INIT)
    {
	DEBUG();

	for (int i=0; i<10; i++)
	{
	    createVideoImage();
	}

	state = OPEN;

	videoDecoder->queue_event(boost::make_shared<OpenVideoOutputResp>());
    }
}

void VideoOutput::process(boost::shared_ptr<CloseVideoOutputReq> event)
{
    if (isOpen())
    {
	DEBUG();

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
    if (isOpen())
    {
	DEBUG();

	xfVideo->resize(event->width, event->height);
	createVideoImage();
    }
}

void VideoOutput::process(boost::shared_ptr<XFVideoImage> event)
{
    if (isOpen())
    {
	DEBUG();

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

void VideoOutput::process(boost::shared_ptr<DeleteXFVideoImage> event)
{
    // This class is running in the GUI thread. Here it is safe to delete
    // X11 resources. The XvImage contained in the XFVideoImage object
    // is deleted by not storing the shared_ptr.
}

void VideoOutput::process(boost::shared_ptr<ShowNextFrame> event)
{
    if (state == PLAYING)
    {
	DEBUG();

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

	    DEBUG(<< "audioSnapshotPTS =" << audioSnapshotPTS
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

void VideoOutput::process(boost::shared_ptr<FlushReq> event)
{
    if (isOpen())
    {
	DEBUG();

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

void VideoOutput::process(boost::shared_ptr<AudioFlushedInd> event)
{
    if (isOpen())
    {
	DEBUG();
	ignoreAudioSync--;
    }
}

void VideoOutput::process(boost::shared_ptr<SeekRelativeReq> event)
{
    event->displayedFramePTS = displayedFramePTS;
    demuxer->queue_event(event);
}

void VideoOutput::process(boost::shared_ptr<EndOfVideoStream> event)
{
    if (isOpen())
    {
	DEBUG();
	eos = true;
	if (frameQueue.empty())
	{
	    mediaPlayer->queue_event(boost::make_shared<EndOfVideoStream>());
	}
    }
}

void VideoOutput::process(boost::shared_ptr<NoAudioStream> event)
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
	DEBUG();

	// Make VideoOutput::sendNotificationVideoSize accessable for XFVideo: 
	typedef void (VideoOutput::*fct_t)(boost::shared_ptr<NotificationVideoSize>);
	fct_t tmp = &VideoOutput::sendNotificationVideoSize;
	boost::function<void (boost::shared_ptr<NotificationVideoSize>)> fct = boost::bind(tmp, this, _1);

	xfVideo = boost::make_shared<XFVideo>(static_cast<Display*>(event->display),
					      event->window, 720, 576, fct);


	showBlackFrame();
    }
}

void VideoOutput::process(boost::shared_ptr<WindowConfigureEvent> event)
{
    DEBUG();
    if (xfVideo)
    {
	xfVideo->handleConfigureEvent();
	if (!isOpen())
	{
	    showBlackFrame();
	}
    }
}

void VideoOutput::process(boost::shared_ptr<WindowExposeEvent> event)
{
    DEBUG();
    if (xfVideo)
    {
	xfVideo->handleExposeEvent();
	if (!isOpen())
	{
	    showBlackFrame();
	}
    }
}

void VideoOutput::process(boost::shared_ptr<ClipVideoEvent> event)
{
    if (xfVideo)
    {
	xfVideo->clip(event->left, event->right, event->top, event->buttom);
    }
}

void VideoOutput::process(boost::shared_ptr<CommandPlay> event)
{
    if (isOpen())
    {
	DEBUG();

	displayNextFrame();
    }
}

void VideoOutput::process(boost::shared_ptr<CommandPause> event)
{
    if (isOpen())
    {
	DEBUG();

	state = PAUSE;
	audioSync = false;
    }
}

void VideoOutput::createVideoImage()
{
#ifdef SYNCTEST
    syncTest->queue_event(boost::make_shared<XFVideoImage>(xfVideo));
#else
    videoDecoder->queue_event(boost::make_shared<XFVideoImage>(xfVideo));
#endif
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

    DEBUG();

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

	INFO(<< "VOUT: currentTime=" <<  currentTime
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
#ifdef SYNCTEST
	syncTest->queue_event(previousImage);
#else
	videoDecoder->queue_event(previousImage);
#endif
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

    DEBUG();

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
	INFO( << "VOUT: startFrameTimer: currentTime=" << currentTime
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
    xfVideo->show(yuvImage);
}

void VideoOutput::sendNotificationVideoSize(boost::shared_ptr<NotificationVideoSize> event)
{
    mediaPlayer->queue_event(event);
}
