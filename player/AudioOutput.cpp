//
// Audio Output
//
// Copyright (C) Joachim Erbs, 2009
//

#include "player/AudioOutput.hpp"
#include "player/AudioDecoder.hpp"
#include "player/VideoOutput.hpp"
#include "player/AlsaFacade.hpp"
#include "player/SyncTest.hpp"
#include "platform/timer.hpp"

#include <boost/make_shared.hpp>

void AudioOutput::process(boost::shared_ptr<InitEvent> event)
{
    if (state == IDLE)
    {
	DEBUG();

#ifdef SYNCTEST
	syncTest = event->syncTest;
#else
	audioDecoder = event->audioDecoder;
#endif
	videoOutput = event->videoOutput;

	state = INIT;
    }
}

void AudioOutput::process(boost::shared_ptr<OpenAudioOutputReq> event)
{
    if (state == INIT)
    {
	sampleRate = event->sample_rate;
	channels = event->channels;
	frameSize = event->frame_size;

	DEBUG(<< "sampleRate=" << sampleRate << ", channels=" << channels << ", frameSize=" << frameSize);

	alsa = boost::make_shared<AFPCMDigitalAudioInterface>(event);

	for (int i=0; i<10; i++)
	{
	    createAudioFrame();
	}

	state = OPEN;

	audioDecoder->queue_event(boost::make_shared<OpenAudioOutputResp>());
    }
}

void AudioOutput::process(boost::shared_ptr<CloseAudioOutputReq> event)
{
    if (isOpen())
    {
	DEBUG();

	alsa->stop();

	// Throw away all queued frames:
	while ( !frameQueue.empty() )
	{
	    frameQueue.pop();
	}

	alsa.reset();

	state = INIT;

	audioDecoder->queue_event(boost::make_shared<CloseAudioOutputResp>());
    }
}

void AudioOutput::process(boost::shared_ptr<AFAudioFrame> event)
{
    if (isOpen())
    {
	DEBUG();

	frameQueue.push(event);

	switch (state)
	{
	case OPEN:
	case STILL:
	case PLAYING:
	    playNextChunk();
	    break;

	default:
	    break;
	}
    }
}

void AudioOutput::process(boost::shared_ptr<PlayNextChunk> event)
{
    if (state == PLAYING)
    {
	DEBUG();

	playNextChunk();
    }
}

void AudioOutput::process(boost::shared_ptr<FlushReq> event)
{
    if (isOpen())
    {
	DEBUG();

	// Flush audio output buffer in driver:
	alsa->stop();

	// Send received frames back to VideoDecoder without showing them:
	while (!frameQueue.empty())
	{
	    boost::shared_ptr<AFAudioFrame> frame(frameQueue.front());
	    frameQueue.pop();
	    frame->reset();
	    audioDecoder->queue_event(frame);
	}

	// Timeout event PlayNextChunk may be received after the timer is
	// stopped. In this case the frameQueue is empty and the event will
	// be ignored.
	stop_timer(chunkTimer);

	// No frame available to display.
	state = OPEN;
    }
}

void AudioOutput::process(boost::shared_ptr<CommandPlay> event)
{
    if (isOpen())
    {
	DEBUG();

	alsa->pause(false);
	playNextChunk();
    }
}

void AudioOutput::process(boost::shared_ptr<CommandPause> event)
{
    if (isOpen())
    {
	DEBUG();

	state = PAUSE;
	alsa->pause(true);
    }
}

void AudioOutput::createAudioFrame()
{
    DEBUG();
#ifdef SYNCTEST
    syncTest->queue_event(boost::make_shared<AFAudioFrame>(frameSize));
#else
    audioDecoder->queue_event(boost::make_shared<AFAudioFrame>(frameSize));
#endif
}

void AudioOutput::playNextChunk()
{
    if (isOpen())
    {
    DEBUG(<< "time=" << chunkTimer.get_current_time());

    while(1)
    {
	if (frameQueue.empty())
	{
	    DEBUG(<< "frameQueue empty");
	    // No frame available to play.
	    state = STILL;
	    return;
	}

	boost::shared_ptr<AFAudioFrame> frame(frameQueue.front());

	bool nextAudioFrame = frame->atBegin();

	bool finished = alsa->play(frame);

	// Here audio is playing and latency can be determined, i.e. sending
	// AudioSyncInfo is possible. VideoOutput needs an AudioSyncInfo when 
	// restarting after pause. Periodic AudioSyncInfo events are necessary
	// to ensure AV-Sync.

	if (state != PLAYING ||   // Restarting after pause.
	    nextAudioFrame)       // Periodic AudioSyncInfo events.
	{
	    sendAudioSyncInfo(alsa->getNextPTS());
	}

	if (finished)
	{
	    frameQueue.pop();
	    frame->reset();
#ifdef SYNCTEST
	    syncTest->queue_event(frame);
#else
	    audioDecoder->queue_event(frame);
#endif
	}
	else
	{
	    // buffer is full
	    break;
	}
    }

    startChunkTimer();
    }
}

void AudioOutput::startChunkTimer()
{
    snd_pcm_sframes_t filled = alsa->getBufferFillLevel();
    double filledInSeconds = double(filled) / double(sampleRate);
    timespec_t dt = getTimespec(filledInSeconds * 0.1);
    timespec_t t_min = getTimespec(0.01);

    if (dt < t_min)
    {
	// Start timer with at least 10ms too avoid busy loop.
	dt = t_min;
    }

    chunkTimer.relative(dt);
    DEBUG(<< "time=" << getSeconds(chunkTimer.get_current_time()) << ", sec=" << getSeconds(dt));
    start_timer(boost::make_shared<PlayNextChunk>(), chunkTimer);

    state = PLAYING;
}

void AudioOutput::sendAudioSyncInfo(double nextPTS)
{
    DEBUG();
    // Do I have to use the frameTimer of class VideoOutput here?
    struct timespec currentTime = chunkTimer.get_current_time();
    snd_pcm_sframes_t overallLatencyInFrames;
    if (alsa->getOverallLatency(overallLatencyInFrames))
    {
	double latencyInSeconds = double(overallLatencyInFrames) / double(sampleRate);
	double currentPTS = nextPTS - latencyInSeconds;

	INFO( << "AOUT: currentTime=" << currentTime
	      << ", currentPTS=" << currentPTS
	      << ", latencyInSeconds=" << latencyInSeconds );

	boost::shared_ptr<AudioSyncInfo> audioSyncInfo(new AudioSyncInfo(currentPTS, currentTime));
	videoOutput->queue_event(audioSyncInfo);
    }
    else
    {
	// It is not necessary to send every audioSyncInfo event.
    }
}
