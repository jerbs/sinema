//
// Audio Decoder
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

#ifndef AUDIO_DECODER_HPP
#define AUDIO_DECODER_HPP

#include "player/GeneralEvents.hpp"
#include "platform/event_receiver.hpp"

class AudioFrame;

class AudioDecoder : public event_receiver<AudioDecoder>
{
    friend class event_processor<>;

    enum state_t {
	Closed,
	Opening,
	Opened,
	Closing
    };
    state_t state;

    AVFormatContext* avFormatContext;
    AVCodecContext* avCodecContext;
    AVCodec* avCodec;
    AVStream* avStream;

    int audioStreamIndex;

    AVPacket avPacket;
    bool avPacketIsFree;

    AVFrame* avFrame;
    bool avFrameIsFree;
    double pts;
    int avFrameBytesTransmittedPerLine;

    AVSampleFormat outputAvSampleFormat;
    int sampleSize;

    // int posCurrentPacket; // Offset in avPacket in packetQueue.front()

    int numFramesCurrentPacket; // Number of samples added to frameQueue.front()

    std::queue<boost::shared_ptr<AudioFrame> > frameQueue;
    std::queue<boost::shared_ptr<AudioPacketEvent> > packetQueue;

    bool eos;

public:
    AudioDecoder(event_processor_ptr_type evt_proc);
    ~AudioDecoder();

private:
    boost::shared_ptr<Demuxer> demuxer;
    boost::shared_ptr<AudioOutput> audioOutput;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<OpenAudioStreamReq> event);
    void process(boost::shared_ptr<OpenAudioOutputResp> event);
    void process(boost::shared_ptr<OpenAudioOutputFail> event);
    void process(boost::shared_ptr<CloseAudioStreamReq> event);
    void process(boost::shared_ptr<CloseAudioOutputResp> event);
    void process(boost::shared_ptr<AudioPacketEvent> event);
    void process(boost::shared_ptr<AudioFrame> event);
    void process(boost::shared_ptr<FlushReq> event);
    void process(boost::shared_ptr<EndOfAudioStream> event);

    void decode();
    void queue();
};

#endif
