#ifndef AUDIO_OUTPUT_HPP
#define AUDIO_OUTPUT_HPP

#include "GeneralEvents.hpp"
#include "event_receiver.hpp"
#include "AlsaFacade.hpp"

#include <queue>

#include <boost/shared_ptr.hpp>

struct PlayNextChunk{};

class AudioOutput : public event_receiver<AudioOutput>
{
    friend class event_processor;

public:
    AudioOutput(event_processor_ptr_type evt_proc);
    ~AudioOutput();

private:
#ifdef SYNCTEST
    boost::shared_ptr<SyncTest> syncTest;
#else
    boost::shared_ptr<AudioDecoder> audioDecoder;
#endif
    boost::shared_ptr<VideoOutput> videoOutput;

    timer chunkTimer;

    boost::shared_ptr<AFPCMDigitalAudioInterface> alsa;
    std::queue<boost::shared_ptr<AFAudioFrame> > frameQueue;

    unsigned int sampleRate;
    unsigned int channels;
    unsigned int frameSize;

    double displayedPTS;
    timespec_t displayedTime;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<StartEvent> event);
    void process(boost::shared_ptr<OpenAudioOutputReq> event);
    void process(boost::shared_ptr<AFAudioFrame> event);
    void process(boost::shared_ptr<PlayNextChunk> event);

    void createAudioFrame();
    void playNextChunk();
    void startChunkTimer();

    void sendAudioSyncInfo(double nextPTS);
};

#endif
