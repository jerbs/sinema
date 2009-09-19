#include "player/AudioOutput.hpp"
#include "player/AudioDecoder.hpp"
#include "player/VideoOutput.hpp"
#include "player/AlsaFacade.hpp"
#include "player/SyncTest.hpp"
#include "platform/timer.hpp"

#include <boost/make_shared.hpp>

AudioOutput::AudioOutput(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      alsa()
{}

AudioOutput::~AudioOutput()
{
}

void AudioOutput::process(boost::shared_ptr<InitEvent> event)
{
    DEBUG();
#ifdef SYNCTEST
    syncTest = event->syncTest;
#else
    audioDecoder = event->audioDecoder;
#endif
    videoOutput = event->videoOutput;
}

void AudioOutput::process(boost::shared_ptr<StartEvent> event)
{
    DEBUG();
}

void AudioOutput::process(boost::shared_ptr<OpenAudioOutputReq> event)
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
}

void AudioOutput::process(boost::shared_ptr<AFAudioFrame> event)
{
    DEBUG();
    frameQueue.push(event);
    playNextChunk();
}

void AudioOutput::process(boost::shared_ptr<PlayNextChunk> event)
{
    DEBUG();
    playNextChunk();
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
    DEBUG(<< "time=" << chunkTimer.get_current_time());

    while(1)
    {
	if (frameQueue.empty())
	{
	    DEBUG(<< "frameQueue empty");
	    // No frame available to play.
	    break;
	    // return;
	}

	boost::shared_ptr<AFAudioFrame> frame(frameQueue.front());

	if (frame->atBegin())
	{
	    DEBUG( << "Frame: byteSize=" << frame->getFrameByteSize() 
		   << ", allocated=" << frame->numAllocatedBytes() );
	    sendAudioSyncInfo(frame->getPTS());
	}

	bool finished = alsa->play(frame);

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
