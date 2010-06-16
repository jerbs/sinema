#include "AudioOutput.hpp"
#include "AudioDecoder.hpp"
#include "AlsaFacade.hpp"

#include <boost/make_shared.hpp>

#undef DEBUG
#define DEBUG(x) std::cout << __PRETTY_FUNCTION__  x << std::endl

AudioOutput::AudioOutput(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      alsa(),
      state(IDLE)
{}

AudioOutput::~AudioOutput()
{
}

void AudioOutput::process(boost::shared_ptr<InitEvent> event)
{
    DEBUG();
    audioDecoder = event->audioDecoder;
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

    switch(state)
    {
    case IDLE:
	playNextChunk();
	state = INIT;
	break;

    case INIT:
	startChunkTimer();
	break;

    default:
	break;
    }
}

void AudioOutput::process(boost::shared_ptr<PlayNextChunk> event)
{
    DEBUG();
    playNextChunk();
}

void AudioOutput::createAudioFrame()
{
    DEBUG();
    audioDecoder->queue_event(boost::make_shared<AFAudioFrame>(frameSize));
}

void AudioOutput::playNextChunk()
{
    DEBUG();
    if (frameQueue.empty())
    {
        // No frame available to play.
        state = INIT;
        return;
    }

    boost::shared_ptr<AFAudioFrame> frame(frameQueue.front());

    bool finished = alsa->play(frame);
    if (finished)
    {
	frameQueue.pop();
	audioDecoder->queue_event(frame);
    }

    startChunkTimer();
}

void AudioOutput::startChunkTimer()
{
    DEBUG();
    // Single shot relative timer:
    timespec_t dt = getTimespec(0.01);
    chunkTimer.relative(dt);
    start_timer(boost::make_shared<PlayNextChunk>(), chunkTimer);

    state = RUNNING;
}
