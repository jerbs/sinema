//
// Video Output
//
// Copyright (C) Joachim Erbs, 2009
//

#include "player/VideoOutput.hpp"
#include "player/VideoDecoder.hpp"
#include "player/MediaPlayer.hpp"

#ifdef SYNCTEST
#include "SyncTest.hpp"
#endif

#include <boost/make_shared.hpp>

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

	xfVideo = boost::make_shared<XFVideo>(event->width, event->height);

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

	// Xv Window is closed.
	// Alternative is showing a black frame or a logo.
	// xfVideo->stop();

	// Throw away all queued frames:
	while ( !frameQueue.empty() )
	{
	    frameQueue.pop();
	}

	xfVideo.reset();

	state = INIT;
	audioSync = false;
	lastNotifiedTime = -1;
	displayedFramePTS = 0;
	ignoreAudioSync = 0;

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
    if (isOpen() && ignoreAudioSync == 0)
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

	// No frame available to display.
	state = STILL;

	// Audio synchronization has to be reestablished:
	audioSync = false;

	ignoreAudioSync++;
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
	// To calculate the timer the next frame and audio synchronization 
	// information must be available.
	state = STILL;
	return;
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
