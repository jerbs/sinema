//
// Video Decoder
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
    std::queue<std::unique_ptr<XFVideoImage> > frameQueue;
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
    void process(  std::unique_ptr<XFVideoImage> event);
    void process(boost::shared_ptr<FlushReq> event);
    void process(boost::shared_ptr<EndOfVideoStream> event);
    void process(boost::shared_ptr<EnableOptimalPixelFormat> event);
    void process(boost::shared_ptr<DisableOptimalPixelFormat> event);

    void decode();
    void queue();

    void setImageFormat(int imageFormat);
    void getSwsContext();
    void requestNewFrame();
};

#endif
