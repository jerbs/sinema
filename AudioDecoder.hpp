#ifndef AUDIO_DECODER_HPP
#define AUDIO_DECODER_HPP

#include "GeneralEvents.hpp"
#include "event_receiver.hpp"
#include "AlsaFacade.hpp"

class AudioDecoder : public event_receiver<AudioDecoder>
{
    friend class event_processor;

    AVFormatContext* avFormatContext;
    AVCodecContext* avCodecContext;
    AVCodec* avCodec;
    AVStream* avStream;  // audio_st

    int audioStreamIndex;

    uint64_t audio_pkt_pts;

    double pts;
    std::queue<boost::shared_ptr<AFAudioFrame> > frameQueue;
    std::queue<boost::shared_ptr<AudioPacketEvent> > packetQueue;

    int posCurrentPacket;

public:
    AudioDecoder(event_processor_ptr_type evt_proc)
	: base_type(evt_proc),
	  avFormatContext(0),
	  avCodecContext(0),
	  avCodec(0),
	  avStream(0),
	  audioStreamIndex(-1),
	  audio_pkt_pts(AV_NOPTS_VALUE),
	  pts(0),
	  posCurrentPacket(0)
    {}
    ~AudioDecoder()
    {}

private:
    boost::shared_ptr<Demuxer> demuxer;
    boost::shared_ptr<AudioOutput> audioOutput;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<StartEvent> event);
    void process(boost::shared_ptr<OpenAudioStreamReq> event);
    void process(boost::shared_ptr<AudioPacketEvent> event);
    void process(boost::shared_ptr<AFAudioFrame> event);

    void decode();
};

#endif
