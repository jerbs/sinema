#ifndef VIDEO_DECODER_HPP
#define VIDEO_DECODER_HPP

#include "player/GeneralEvents.hpp"
#include "platform/event_receiver.hpp"
#include "player/XlibFacade.hpp"

#include <boost/shared_ptr.hpp>

class VideoDecoder : public event_receiver<VideoDecoder>
{
    friend class event_processor;

    AVFormatContext* avFormatContext;
    AVCodecContext* avCodecContext;
    AVCodec* avCodec;
    AVStream* avStream;

    int videoStreamIndex;

    uint64_t video_pkt_pts;

    AVFrame* avFrame;
    bool avFrameIsFree;
    double pts;
    std::queue<boost::shared_ptr<XFVideoImage> > frameQueue;
    std::queue<boost::shared_ptr<VideoPacketEvent> > packetQueue;

    struct SwsContext* swsContext;

public:
    VideoDecoder(event_processor_ptr_type evt_proc)
	: base_type(evt_proc),
	  avFormatContext(0),
	  avCodecContext(0),
	  avCodec(0),
	  avStream(0),
	  videoStreamIndex(-1),
	  video_pkt_pts(AV_NOPTS_VALUE),
	  avFrame(avcodec_alloc_frame()),
	  avFrameIsFree(true),
	  pts(0),
	  swsContext(0)
    {}
    ~VideoDecoder()
    {
	if (avFrame)
	{
	    av_free(avFrame);
	}

	if (swsContext)
	{
	    sws_freeContext(swsContext);
	}
    }

private:
    boost::shared_ptr<Demuxer> demuxer;
    boost::shared_ptr<VideoOutput> videoOutput;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<StartEvent> event);
    void process(boost::shared_ptr<OpenVideoStreamReq> event);
    void process(boost::shared_ptr<VideoPacketEvent> event);
    void process(boost::shared_ptr<XFVideoImage> event);

    void decode();
    void queue();
};

#endif
