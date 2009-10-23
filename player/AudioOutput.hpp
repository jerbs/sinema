//
// Audio Output
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef AUDIO_OUTPUT_HPP
#define AUDIO_OUTPUT_HPP

#include "player/GeneralEvents.hpp"
#include "platform/event_receiver.hpp"
#include "player/AlsaFacade.hpp"
#include "player/AlsaMixer.hpp"

#include <queue>

#include <boost/shared_ptr.hpp>

struct PlayNextChunk{};

class AudioOutput : public event_receiver<AudioOutput>
{
    friend class event_processor<>;

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
    MediaPlayer* mediaPlayer;

    timer chunkTimer;

    boost::shared_ptr<AFPCMDigitalAudioInterface> alsa;
    boost::shared_ptr<AFMixer> alsaMixer;
    std::queue<boost::shared_ptr<AFAudioFrame> > frameQueue;

    bool eos;

    typedef enum {
	IDLE,
	INIT,
	OPEN,
	PAUSE,
	STILL,
	PLAYING
    } state_t;

    state_t state;

    bool isOpen() {return (state >= OPEN) ? true : false;}

    unsigned int sampleRate;
    unsigned int channels;
    unsigned int frameSize;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<OpenAudioOutputReq> event);
    void process(boost::shared_ptr<CloseAudioOutputReq> event);
    void process(boost::shared_ptr<AFAudioFrame> event);
    void process(boost::shared_ptr<PlayNextChunk> event);
    void process(boost::shared_ptr<FlushReq> event);
    void process(boost::shared_ptr<EndOfAudioStream> event);
    void process(boost::shared_ptr<AlsaMixerElemEvent> event);

    void process(boost::shared_ptr<CommandPlay> event);
    void process(boost::shared_ptr<CommandPause> event);
    void process(boost::shared_ptr<CommandSetPlaybackVolume> event);
    void process(boost::shared_ptr<CommandSetPlaybackSwitch> event);

    void createAudioFrame();
    void playNextChunk();
    void startChunkTimer();
    bool startEosTimer();

    void sendAudioSyncInfo(double nextPTS);
};

#endif
