//
// Audio Output
//
// Copyright (C) Joachim Erbs, 2009, 2010
//

#include "player/AudioOutput.hpp"
#include "player/AudioDecoder.hpp"
#include "player/VideoOutput.hpp"
#include "player/MediaPlayer.hpp"
#include "player/AlsaFacade.hpp"
#include "player/AlsaMixer.hpp"
#include "platform/timer.hpp"

#include <boost/make_shared.hpp>
#include <sys/types.h>

AudioOutput::AudioOutput(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      mediaPlayer(0),
      currentFrame(frameQueue.end()),
      state(IDLE),
      eos(false),
      audioStreamOnly(false),
      lastNotifiedTime(-1),
      audiblePTS(-1),
      numAudioFrames(0),
      numPostBuffered(0)
{
}

AudioOutput::~AudioOutput()
{
}

void AudioOutput::process(boost::shared_ptr<InitEvent> event)
{
    if (state == IDLE)
    {
	TRACE_DEBUG(<< "tid = " << gettid());

	audioDecoder = event->audioDecoder;
	videoOutput = event->videoOutput;
	mediaPlayer = event->mediaPlayer;

	alsaMixer = boost::make_shared<AFMixer>(this, mediaPlayer);

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

	TRACE_DEBUG(<< "sampleRate=" << sampleRate << ", channels=" << channels << ", frameSize=" << frameSize);

	alsa = boost::make_shared<AFPCMDigitalAudioInterface>(event);

	// Make AudioOutput::sendAudioSyncInfo accessable for AFPCMDigitalAudioInterface: 
	typedef void (AudioOutput::*fct_t)();
	fct_t tmp = &AudioOutput::sendAudioSyncInfo;
	alsa->setSendAudioSyncInfo(boost::bind(tmp, this));

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
	TRACE_DEBUG();

	alsa->stop();

	// Throw away all queued frames:
	frameQueue.clear();
	currentFrame = frameQueue.end();
	numAudioFrames = 0;
	numPostBuffered = 0;

	alsa.reset();

	state = INIT;

	audioStreamOnly = false;
	lastNotifiedTime = -1;

	audioDecoder->queue_event(boost::make_shared<CloseAudioOutputResp>());
    }
}

void AudioOutput::process(boost::shared_ptr<AFAudioFrame> event)
{
    if (isOpen())
    {
	TRACE_DEBUG();

	eos = false;
	frameQueue.push_back(event);
	if (currentFrame == frameQueue.end())
	{
	    currentFrame--;
	}

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
	TRACE_DEBUG();

	playNextChunk();
    }
}

void AudioOutput::process(boost::shared_ptr<FlushReq> event)
{
    if (isOpen())
    {
	TRACE_DEBUG();

	// Flush audio output buffer in driver:
	alsa->stop();

	// Send received frames back to VideoDecoder without playing them:
	while (!frameQueue.empty())
	{
	    boost::shared_ptr<AFAudioFrame> frame(frameQueue.front());
	    frameQueue.pop_front();
	    frame->reset();

	    audioDecoder->queue_event(frame);
	}
	currentFrame = frameQueue.end();
	numPostBuffered = 0;

	// Timeout event PlayNextChunk may be received after the timer is
	// stopped. In this case the frameQueue is empty and the event will
	// be ignored.
	stop_timer(chunkTimer);

	// No frame available to display.
	state = OPEN;

	// Notify VideoOutput about flushed AudioOutput:
	videoOutput->queue_event(boost::make_shared<AudioFlushedInd>());
    }
}

void AudioOutput::process(boost::shared_ptr<EndOfAudioStream> event)
{
    if (isOpen())
    {
	TRACE_DEBUG();
	eos = true;
	if (frameQueue.empty())
	{
	    if (!startEosTimer())
	    {
		mediaPlayer->queue_event(boost::make_shared<EndOfAudioStream>());
	    }
	}
    }
}

void AudioOutput::process(boost::shared_ptr<NoVideoStream> event)
{
    audioStreamOnly = true;
    // No timer has to be started here. Audio plays without video by default.
}

void AudioOutput::process(boost::shared_ptr<AlsaMixerElemEvent> event)
{
    if (alsaMixer)
    	alsaMixer->process(event);
}

void AudioOutput::process(boost::shared_ptr<CommandPlay> event)
{
    if (isOpen())
    {
	TRACE_DEBUG();

	if (state == PAUSE)
	{
	    if (!alsa->pause(false))
	    {
		// Audio device does not support pause.
		// Go back in frame queue, to refill the buffer in the 
		// audio device with the dropped frames.
		while(currentFrame != frameQueue.begin())
		{
		    boost::shared_ptr<AFAudioFrame> frame(*currentFrame);
		    if (frame->getPTS() > audiblePTS)
		    {
			// Complete frame has to be replayed.
			frame->restore();
			currentFrame--;
		    }
		    else
		    {
			// Partial frame has to be replayed.
			boost::shared_ptr<AFAudioFrame> frame(*currentFrame);
			frame->restore();
			double seconds = audiblePTS - frame->getPTS();
			alsa->skip(frame, seconds);
			break;
		    }
		}
	    }
	}

	playNextChunk();
    }
}

void AudioOutput::process(boost::shared_ptr<CommandPause> event)
{
    if (isOpen())
    {
	TRACE_DEBUG();

	snd_pcm_sframes_t overallLatencyInFrames;
	if (alsa->getOverallLatency(overallLatencyInFrames))
	{
	    double latencyInSeconds = double(overallLatencyInFrames) / double(sampleRate);
	    audiblePTS = alsa->getNextPTS() - latencyInSeconds;
	}

	state = PAUSE;
	alsa->pause(true);
    }
}

void AudioOutput::process(boost::shared_ptr<CommandSetPlaybackVolume> event)
{
    if (alsaMixer)
	alsaMixer->setPlaybackVolume(event->volume);
}

void AudioOutput::process(boost::shared_ptr<CommandSetPlaybackSwitch> event)
{
    if (alsaMixer)
	alsaMixer->setPlaybackSwitch(event->enabled);
}

void AudioOutput::createAudioFrame()
{
    TRACE_DEBUG();
    numAudioFrames++;
    audioDecoder->queue_event(boost::make_shared<AFAudioFrame>(frameSize));
}

void AudioOutput::playNextChunk()
{
    if (isOpen())
    {
    while(1)
    {
	if (currentFrame == frameQueue.end())
	{
	    // No frame available to play.
	    state = STILL;
	    if (eos)
	    {
		// In case of a very short file, playback may not have been started
		// automatically here by AFPCMDigitalAudioInterface.
		alsa->start();

		if (!startEosTimer())
		{
		    // Timer is not started again.
		    // I.e. the audio device has played all samples.
		    mediaPlayer->queue_event(boost::make_shared<EndOfAudioStream>());
		}
	    }

	    recycleObsoleteFrames();
	    return;
	}

	TRACE_DEBUG(<< "time=" << chunkTimer.get_current_time());

	boost::shared_ptr<AFAudioFrame> frame(*currentFrame);

	if (frame->atBegin())
	{
	    // Send periodic AudioSyncInfo event to ensure AV-Sync:
	    TRACE_DEBUG(<< "calling sendAudioSyncInfo");
	    sendAudioSyncInfo();
	}

	bool finished = alsa->play(frame);

	if (finished)
	{
	    // Proceed to next frame in list:
	    currentFrame++;
	    numPostBuffered++;
	    if (numPostBuffered + 10 > numAudioFrames)
	    {
		// Create additional frames.
		createAudioFrame();
	    }
	}
	else
	{
	    // buffer is full
	    break;
	}
    }

    startChunkTimer();
    }

    recycleObsoleteFrames();
}

void AudioOutput::recycleObsoleteFrames()
{
    // Remove obsolete frames from bufferedFrameQueue and send them back to
    // the AudioDecoder.
    FrameQueue_t::iterator it(frameQueue.begin());
    while(it != currentFrame)
    {
	boost::shared_ptr<AFAudioFrame> firstFrame(*it);
	double nextPTS = firstFrame->getNextPTS();
	if (audiblePTS > nextPTS)
	{
	    // Frame not necessary for an restart after pause anymore.
	    it = frameQueue.erase(it);
	    numPostBuffered--;

	    firstFrame->reset();

	    audioDecoder->queue_event(firstFrame);
	}
	else
	{
	    break;
	}
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
    TRACE_DEBUG(<< "time=" << chunkTimer.get_current_time() << ", sec=" << getSeconds(dt));
    start_timer(boost::make_shared<PlayNextChunk>(), chunkTimer);

    state = PLAYING;
}

bool AudioOutput::startEosTimer()
{
    // This function tries to guess when the last sample will be played by
    // the DAC on the sound card.
    // AFPCMDigitalAudioInterface::getOverallLatency returns the time when
    // the next sample written to the buffer will be audible. This is not
    // the same. According to the ALSA documentation the value will not
    // necessarily go down to 0. For the virtual Alsa device of PulseAudio
    // this actually is the case.
    // As an alternative the number of samples still in the playback buffer
    // is used to calculate the time. This does not take the additional
    // hardware delay into account. That delay currently is ignored.

    snd_pcm_sframes_t filled = alsa->getBufferFillLevel();
    if (filled)
    {
	// Audio device is still playing the last frames.
	state = PLAYING;
	if (filled > 1)
	{
	    // When the virtual Alsa device of PulseAudio is used filled 
	    // stays at 1. Zero is not reached. When using the Alsa device 
	    // directly then it is.
	    double filledInSeconds = double(filled) / double(sampleRate);
	    timespec_t dt = getTimespec(filledInSeconds);
	    chunkTimer.relative(dt);
	    TRACE_DEBUG(<< "start_timer filledInSeconds=" << filledInSeconds);
	    start_timer(boost::make_shared<PlayNextChunk>(), chunkTimer);
	    return true;
	}
    }

    return false;
}

void AudioOutput::sendAudioSyncInfo()
{
    double nextPTS = alsa->getNextPTS();
    // Do I have to use the frameTimer of class VideoOutput here?
    struct timespec currentTime = chunkTimer.get_current_time();
    snd_pcm_sframes_t overallLatencyInFrames;
    if (alsa->getOverallLatency(overallLatencyInFrames))
    {
	TRACE_DEBUG();
	double latencyInSeconds = double(overallLatencyInFrames) / double(sampleRate);
	double currentPTS = nextPTS - latencyInSeconds;
	audiblePTS = currentPTS;

	TRACE_INFO( << "AOUT: currentTime=" << currentTime
		    << ", currentPTS=" << currentPTS
		    << ", latencyInSeconds=" << latencyInSeconds );

	boost::shared_ptr<AudioSyncInfo> audioSyncInfo(new AudioSyncInfo(currentPTS, currentTime));
	videoOutput->queue_event(audioSyncInfo);

	// For audio only files send an event to the GUI to update the displayed time.
	// This solution isn't perfect, since sendAudioSyncInfo is not called on second boundaries.
	int currentTime = currentPTS;
	if (audioStreamOnly && currentTime != lastNotifiedTime)
	{
	    mediaPlayer->queue_event(audioSyncInfo);
	    lastNotifiedTime = currentTime;
	}
    }
    else
    {
	// It is not necessary to send every audioSyncInfo event.
    }
}
