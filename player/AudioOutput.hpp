//
// Audio Output
//
// Copyright (C) Joachim Erbs, 2009-2010
//
//    This file is part of Sinema.
//
//    Sinema is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Sinema is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Sinema.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef AUDIO_OUTPUT_HPP
#define AUDIO_OUTPUT_HPP

#include "player/GeneralEvents.hpp"
#include "platform/event_receiver.hpp"

#include <list>

#include <boost/shared_ptr.hpp>

class AFPCMDigitalAudioInterface;
class AudioFrame;
class AFMixer;
class AlsaMixerElemEvent;

struct PlayNextChunk{};

class AudioOutput : public event_receiver<AudioOutput>
{
    friend class event_processor<>;

public:
    AudioOutput(event_processor_ptr_type evt_proc);
    ~AudioOutput();

private:
    boost::shared_ptr<AudioDecoder> audioDecoder;
    boost::shared_ptr<VideoOutput> videoOutput;
    MediaPlayer* mediaPlayer;

    timer chunkTimer;

    boost::shared_ptr<AFPCMDigitalAudioInterface> alsa;
    boost::shared_ptr<AFMixer> alsaMixer;
    typedef std::list<boost::shared_ptr<AudioFrame> > FrameQueue_t;
    FrameQueue_t frameQueue;
    FrameQueue_t::iterator currentFrame;

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

    bool audioStreamOnly;
    int lastNotifiedTime;

    double audiblePTS;
    int numAudioFrames;
    int numPostBuffered;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<OpenAudioOutputReq> event);
    void process(boost::shared_ptr<CloseAudioOutputReq> event);
    void process(boost::shared_ptr<AudioFrame> event);
    void process(boost::shared_ptr<PlayNextChunk> event);
    void process(boost::shared_ptr<FlushReq> event);
    void process(boost::shared_ptr<EndOfAudioStream> event);
    void process(boost::shared_ptr<NoVideoStream> event);
    void process(boost::shared_ptr<AlsaMixerElemEvent> event);

    void process(boost::shared_ptr<CommandPlay> event);
    void process(boost::shared_ptr<CommandPause> event);
    void process(boost::shared_ptr<CommandSetPlaybackVolume> event);
    void process(boost::shared_ptr<CommandSetPlaybackSwitch> event);

    void createAudioFrame();
    void playNextChunk();
    void recycleObsoleteFrames();
    void startChunkTimer();
    bool startEosTimer();

    void sendAudioSyncInfo();
};

#endif
