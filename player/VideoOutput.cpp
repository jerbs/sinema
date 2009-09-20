//
// Video Output
//
// Copyright (C) Joachim Erbs, 2009
//

#include "player/VideoOutput.hpp"
#include "player/VideoDecoder.hpp"

#ifdef SYNCTEST
#include "SyncTest.hpp"
#endif

#include <boost/make_shared.hpp>

using namespace std;

void VideoOutput::process(boost::shared_ptr<InitEvent> event)
{
    DEBUG();

    if (state == IDLE)
    {
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
    DEBUG();

    if (state == INIT)
    {
	xfVideo = boost::make_shared<XFVideo>(event->width, event->height);

	for (int i=0; i<10; i++)
	{
	    createVideoImage();
	}

	state = OPEN;
    }
}

void VideoOutput::process(boost::shared_ptr<ResizeVideoOutputReq> event)
{
    DEBUG();

    if (isOpen())
    {
	xfVideo->resize(event->width, event->height);
	createVideoImage();
    }
}

void VideoOutput::process(boost::shared_ptr<XFVideoImage> event)
{
    DEBUG();

    if (isOpen())
    {
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
    DEBUG();
    if (state == PLAYING)
    {
	displayNextFrame();
    }
}

void VideoOutput::process(boost::shared_ptr<AudioSyncInfo> event)
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

void VideoOutput::process(boost::shared_ptr<CommandPlay> event)
{
    if (isOpen())
    {
	displayNextFrame();
    }
}

void VideoOutput::process(boost::shared_ptr<CommandPause> event)
{
    if (isOpen())
    {
	state = PAUSE;
	audioSync = false;
    }
}

void VideoOutput::process(boost::shared_ptr<CommandStop> event)
{
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
    DEBUG();

    if (frameQueue.empty())
    {
	// No frame available to display.
	state = STILL;
	return;
    }

    boost::shared_ptr<XFVideoImage> image(frameQueue.front());
    frameQueue.pop();

    {
	// For debugging only:
	double displayedFramePTS = image->getPTS();
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
    DEBUG();

    if (frameQueue.empty() || !audioSync)
    {
	// To calculate the timer the next frame and audio synchronization 
	// information must be available.
	state = STILL;
	return;
    }

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
