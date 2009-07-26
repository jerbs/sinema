#ifndef VIDEO_DECODER_HPP
#define VIDEO_DECODER_HPP

#include "GeneralEvents.hpp"
#include "event_receiver.hpp"

class VideoDecoder : public event_receiver<VideoDecoder>
{
    friend class event_processor;

    AVFormatContext* avFormatContext;
    AVCodecContext* avCodecContext;
    AVCodec* avCodec;
    AVStream* avStream;

    int videoStreamIndex;

public:
    VideoDecoder(event_processor_ptr_type evt_proc)
	: base_type(evt_proc),
	  avFormatContext(0),
	  avCodecContext(0),
	  avCodec(0),
	  avStream(0),
	  videoStreamIndex(-1)
    {}
    ~VideoDecoder() {}

private:
    boost::shared_ptr<Demuxer> demuxer;
    boost::shared_ptr<VideoOutput> videoOutput;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<StartEvent> event);
    void process(boost::shared_ptr<OpenVideoStreamReq> event);
    void process(boost::shared_ptr<VideoPacketEvent> event);
};

#endif
