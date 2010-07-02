//
// Video Decoder
//
// Copyright (C) Joachim Erbs, 2009-2010
//

#ifndef VIDEO_DECODER_HPP
#define VIDEO_DECODER_HPP

#include "player/GeneralEvents.hpp"
#include "platform/event_receiver.hpp"

#include <boost/shared_ptr.hpp>

class XFVideoImage;

class VideoDecoder : public event_receiver<VideoDecoder>
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

    int videoStreamIndex;

    uint64_t video_pkt_pts;

    AVFrame* avFrame;
    bool avFrameIsFree;
    double pts;
    int m_imageFormat;
    std::queue<boost::shared_ptr<XFVideoImage> > frameQueue;
    std::queue<boost::shared_ptr<VideoPacketEvent> > packetQueue;

    bool eos;

    struct SwsContext* swsContext;

    bool m_topFieldFirst;

    bool m_useOptimumImageFormat;

public:
    VideoDecoder(event_processor_ptr_type evt_proc);
    ~VideoDecoder();

private:
    MediaPlayer* mediaPlayer;
    boost::shared_ptr<Demuxer> demuxer;
    boost::shared_ptr<VideoOutput> videoOutput;
    boost::shared_ptr<Deinterlacer> deinterlacer;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<OpenVideoStreamReq> event);
    void process(boost::shared_ptr<OpenVideoOutputResp> event);
    void process(boost::shared_ptr<OpenVideoOutputFail> event);
    void process(boost::shared_ptr<CloseVideoStreamReq> event);
    void process(boost::shared_ptr<CloseVideoOutputResp> event);
    void process(boost::shared_ptr<VideoPacketEvent> event);
    void process(boost::shared_ptr<XFVideoImage> event);
    void process(boost::shared_ptr<FlushReq> event);
    void process(boost::shared_ptr<EndOfVideoStream> event);		 

    void decode();
    void queue();

    void getSwsContext();
    void requestNewFrame();
};

#endif
