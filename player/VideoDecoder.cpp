//
// Video Decoder
//
// Copyright (C) Joachim Erbs, 2009-2010
//

#include "player/VideoDecoder.hpp"
#include "player/VideoOutput.hpp"
#include "player/Demuxer.hpp"
#include "player/MediaPlayer.hpp"
#include "player/Deinterlacer.hpp"
#include "player/XlibFacade.hpp"
#include "player/XlibHelpers.hpp"

#include <boost/make_shared.hpp>
#include <iomanip>
#include <sys/types.h>

using namespace std;

void VideoDecoder::process(boost::shared_ptr<InitEvent> event)
{
    DEBUG(<< "tid = " << gettid());
    mediaPlayer = event->mediaPlayer;
    demuxer = event->demuxer;
    videoOutput = event->videoOutput;
    deinterlacer = event->deinterlacer;
}

void VideoDecoder::process(boost::shared_ptr<OpenVideoStreamReq> event)
{
    if (state == Closed)
    {
	DEBUG(<< "streamIndex = " << event->streamIndex);

    videoStreamIndex = event->streamIndex;
    avFormatContext = event->avFormatContext;

    if (videoStreamIndex >= 0 &&
	videoStreamIndex <= (int)avFormatContext->nb_streams)
    {
	// Get a pointer to the codec context for the stream
	avCodecContext = avFormatContext->streams[videoStreamIndex]->codec;

	if (avCodecContext->codec_type == CODEC_TYPE_VIDEO)
	{
	    avCodec = avcodec_find_decoder(avCodecContext->codec_id);
	    if (avCodec)
	    {
		int ret = avcodec_open(avCodecContext, avCodec);
		if (ret == 0)
		{
		    avStream = avFormatContext->streams[videoStreamIndex];

		    int width = avCodecContext->width;
		    int height = avCodecContext->height;
		    // planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
		    // enum PixelFormat dstPixFmt = PIX_FMT_YUV420P;
		    // packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr (needed by deinterlacers)
		    enum PixelFormat dstPixFmt = PIX_FMT_YUYV422;

		    swsContext = sws_getContext(width, height,            // Source Size
						avCodecContext->pix_fmt,  // Source Format
						width, height,            // Destination Size
						PixelFormat(dstPixFmt),   // Destination Format
						SWS_BICUBIC,              // Flags
						NULL, NULL, NULL);        // SwsFilter*
		    if (swsContext)
		    {
			int w = avCodecContext->width;
			int h = avCodecContext->height;
			AVRational& par = avCodecContext->sample_aspect_ratio;
			int pn = par.num;
			int pd = par.den;
			DEBUG( << "size = " << w << ":" << h);
			DEBUG( << "par  = " << pn << ":" << pd);
			if (pn == 0 || pd == 0) {pn = pd = 1;}

			videoOutput->queue_event(boost::make_shared<OpenVideoOutputReq>(w,h,pn,pd));

			state = Opening;

			return;
		    }
		    else
		    {
			ERROR(<< "sws_getContext failed");
		    }
		}
		else
		{
		    ERROR(<< "avcodec_open failed: ret = " << ret);
		}
	    }
	    else
	    {
		ERROR(<< "avcodec_find_decoder failed");
	    }
	}
	else
	{
	    ERROR(<< "expected video stream");
	}
    }
    else
    {
	ERROR(<< "Invalid streamIndex = " << videoStreamIndex);
    }

    avFormatContext = 0;
    avCodecContext = 0;
    avCodec = 0;
    avStream = 0;
    videoStreamIndex = -1;
    eos = false;

    }

    demuxer->queue_event(boost::make_shared<OpenVideoStreamFail>(videoStreamIndex));
}

void VideoDecoder::process(boost::shared_ptr<OpenVideoOutputResp> event)
{
    if (state == Opening)
    {
	DEBUG();

	demuxer->queue_event(boost::make_shared<OpenVideoStreamResp>(videoStreamIndex));
	state = Opened;
    }
}

void VideoDecoder::process(boost::shared_ptr<OpenVideoOutputFail> event)
{
    if (state == Opening)
    {
	DEBUG();

	avcodec_close(avCodecContext);

	avFormatContext = 0;
	avCodecContext = 0;
	avCodec = 0;
	avStream = 0;
	videoStreamIndex = -1;

	demuxer->queue_event(boost::make_shared<OpenVideoStreamFail>(videoStreamIndex));

	state = Closed;
    }
}

void VideoDecoder::process(boost::shared_ptr<CloseVideoStreamReq> event)
{
    if (state != Closed)
    {
	DEBUG();

	if (state == Opened)
	{
	    avcodec_close(avCodecContext);
	}

	// Throw away all queued packets:
	while (!packetQueue.empty())
	{
	    packetQueue.pop();
	}

	// Throw away all queued frames:
	while (!frameQueue.empty())
	{
	    boost::shared_ptr<XFVideoImage> xfVideoImage(frameQueue.front());
	    frameQueue.pop();
	    videoOutput->queue_event(boost::make_shared<DeleteXFVideoImage>(xfVideoImage));
	}

	// Do the same as the Deinterlacer when receiving CloseVideoOutputReq:
	m_topFieldFirst = true;

	// Send event via Deinterlacer to VideoOutput:
	deinterlacer->queue_event(boost::make_shared<CloseVideoOutputReq>());

	state = Closing;
    }
}

void VideoDecoder::process(boost::shared_ptr<CloseVideoOutputResp> event)
{
    if (state == Closing)
    {
	DEBUG();

	avFormatContext = 0;
	avCodecContext = 0;
	avCodec = 0;
	avStream = 0;
	videoStreamIndex = -1;

	video_pkt_pts = AV_NOPTS_VALUE;

	// Keep avFrame.
	avFrameIsFree = true;
	pts = 0;

	demuxer->queue_event(boost::make_shared<CloseVideoStreamResp>());

	state = Closed;
    }
}

void VideoDecoder::process(boost::shared_ptr<VideoPacketEvent> event)
{
    if (state == Opened)
    {
	DEBUG();

	eos = false;
	packetQueue.push(event);
	decode();
    }
}

void VideoDecoder::process(boost::shared_ptr<XFVideoImage> event)
{
    if (state == Opened || state == Opening)
    {
	DEBUG();

	frameQueue.push(event);
	queue();
	decode();
    }
}

void VideoDecoder::process(boost::shared_ptr<FlushReq> event)
{
    if (state == Opened)
    {
	DEBUG();

	// Flush buffers in ffmpeg decoder:
	avcodec_flush_buffers(avCodecContext);

	// Throw away everything received from the Demuxer:
	boost::shared_ptr<ConfirmVideoPacketEvent> confirm(new ConfirmVideoPacketEvent());
	while (!packetQueue.empty())
	{
	    packetQueue.pop();
	    demuxer->queue_event(confirm);
	}

	avFrameIsFree = true;
	video_pkt_pts = AV_NOPTS_VALUE;
	pts = 0;

	// Forward event via Deinterlacer to VideoOutput:
	deinterlacer->queue_event(event);
    }
}

void VideoDecoder::process(boost::shared_ptr<EndOfVideoStream> event)
{
    if (state == Opened)
    {
	DEBUG();
	eos = true;

	if (packetQueue.empty())
	{
	    // Decoded everything in this stream.

	    // Forward event via Deinterlacer to VideoOutput:
	    deinterlacer->queue_event(event);
	    eos = false;
	}
    }
}

std::ostream& operator<<(std::ostream& strm, AVRational r)
{
    strm << r.num << "/" << r.den;
    return strm;
}

void VideoDecoder::decode()
{
    if (!avFrameIsFree)
    {
	DEBUG(<< "frame not yet queued");
	// Wait until current frame is transmitted to VideoOutput.
	return;
    }

    while (!packetQueue.empty())
    {
	boost::shared_ptr<VideoPacketEvent> videoPacketEvent(packetQueue.front());
	packetQueue.pop();

	DEBUG(<< "Queueing ConfirmVideoPacketEvent");
	demuxer->queue_event(boost::make_shared<ConfirmVideoPacketEvent>());

	AVPacket& avPacket = videoPacketEvent->avPacket;
	video_pkt_pts = avPacket.pts;

	int frameFinished;
	int ret = avcodec_decode_video(avCodecContext, avFrame, &frameFinished,
				       avPacket.data, avPacket.size);

	if (ret>=0)
	{
	    if (avPacket.dts == (int64_t)AV_NOPTS_VALUE &&
		avFrame->opaque &&
		*(int64_t*)avFrame->opaque != AV_NOPTS_VALUE)
	    {
		pts = *(uint64_t *)avFrame->opaque;
	    }
	    else if (avPacket.dts != (int64_t)AV_NOPTS_VALUE)
	    {
		pts = avPacket.dts;
	    }
	    else
	    {
		pts = 0;
	    }
	    pts *= av_q2d(avStream->time_base);

	    static int64_t lastDts = 0;

	    INFO( << "VDEC: pts=" << std::fixed << std::setprecision(2) << pts 
		  << ", pts=" << avFrame->pts
		  << ", dts=" << avPacket.dts << "(" << avPacket.dts-lastDts << ")"
		  << ", time_base=" << avStream->time_base
		  << ", frameFinished=" << frameFinished );

	    lastDts = avPacket.dts;
	    
	    if (frameFinished)
	    {
		avFrameIsFree = false;
		queue();
		if (!avFrameIsFree)
		{
		    // Call queue() again when a frame is available.
		    return;
		}
	    }
	}
	else
	{
	    ERROR(<< "avcodec_decode_video failed");
	}
    }

    if (eos && avFrameIsFree && packetQueue.empty())
    {
	// Decoded everything in this stream.

	// Forward event via Deinterlacer to VideoOutput:
	DEBUG(<< "forwarding EndOfVideoStream");
	deinterlacer->queue_event(boost::make_shared<EndOfVideoStream>());
	eos = false;
    }

}

void VideoDecoder::queue()
{
    if (avFrameIsFree)
    {
	// Nothing to queue
	DEBUG(<< "nothing to queue");
	return;
    }

    if (frameQueue.empty())
    {
	DEBUG(<< "frameQueue.empty() returned true");
	return;
    }

    if (avFrame->interlaced_frame && frameQueue.size() < 2)
    {
	return;
    }

    boost::shared_ptr<XFVideoImage> xfVideoImage(frameQueue.front());
    frameQueue.pop();

    unsigned int width = xfVideoImage->width();
    unsigned int height = xfVideoImage->height();

    if (avCodecContext->width != int(width) ||
	avCodecContext->height != int(height))
    {
	DEBUG(<< "Frame buffer with wrong size."
	      << " needed:" << avCodecContext->width << "*" << avCodecContext->height
	      << " got: " << width << "*" << height);

	videoOutput->queue_event(boost::make_shared<DeleteXFVideoImage>(xfVideoImage));

	int w = avCodecContext->width;
	int h = avCodecContext->height;
	AVRational& par = avCodecContext->sample_aspect_ratio;
	int pn = par.num;
	int pd = par.den;
	if (pn == 0 || pd == 0) {pn = pd = 1;}
	videoOutput->queue_event(boost::make_shared<ResizeVideoOutputReq>(w,h,pn,pd));

	return;
    }

    DEBUG( << "interlaced_frame = " << avFrame->interlaced_frame
	   << ", top_field_first = " << avFrame->top_field_first);

    XvImage* yuvImage = xfVideoImage->xvImage();
    char* data = yuvImage->data;

    AVPicture avPicture;

    if (yuvImage->id == GUID_YUV12_PLANAR)
    {
	char* Y = data + yuvImage->offsets[0];
	char* V = data + yuvImage->offsets[1];
	char* U = data + yuvImage->offsets[2];

	int Yp = yuvImage->pitches[0];
	int Vp = yuvImage->pitches[1];
	int Up = yuvImage->pitches[2];

	avPicture.data[0] = (uint8_t*)Y;
	avPicture.data[1] = (uint8_t*)U;
	avPicture.data[2] = (uint8_t*)V;
	avPicture.data[3] = 0;
	avPicture.linesize[0] = Yp;
	avPicture.linesize[1] = Up;
	avPicture.linesize[2] = Vp;
	avPicture.linesize[3] = 0;
    }
    else if (yuvImage->id == GUID_YUY2_PACKED)
    {
	char* P = data + yuvImage->offsets[0];
	int Pp = yuvImage->pitches[0];
	avPicture.data[0] = (uint8_t*)P;
	avPicture.data[1] = 0;
	avPicture.data[2] = 0;
	avPicture.data[3] = 0;
	avPicture.linesize[0] = Pp;
	avPicture.linesize[1] = 0;
	avPicture.linesize[2] = 0;
	avPicture.linesize[3] = 0;
    }
    else
    {
	ERROR(<< "unsupported format 0x" << std::hex << yuvImage->id);
    }

    // Convert image into YUV format:
    sws_scale(swsContext,
	      avFrame->data,            // srcSlice:  Array with pointers to planes
	      avFrame->linesize,        // srcStride: Array with strides of each plane
	      0,                        // srcSliceY: position
	      avCodecContext->height,   // srcSliceH: height of the source slice
	      avPicture.data,                // dstSlice:  Array with pointers to planes
	      avPicture.linesize);           // dstStride: Array with strides of each plane

    xfVideoImage->setPTS(pts);

    if (avFrame->interlaced_frame && yuvImage->id == GUID_YUY2_PACKED)
    {
	bool tff = avFrame->top_field_first ? true : false;
	if (m_topFieldFirst != tff)
	{
	    // Deinterlacer needs an update.
	    if (tff)
	    {
		deinterlacer->queue_event(boost::make_shared<TopFieldFirst>());
	    }
	    else
	    {
		deinterlacer->queue_event(boost::make_shared<BottomFieldFirst>());
	    }
	}

	// The deinterlacer needs two frames.
	deinterlacer->queue_event(xfVideoImage);

	boost::shared_ptr<XFVideoImage> xfVideoImage2(frameQueue.front());
	frameQueue.pop();

	// FIXME: Size check is missing.
	deinterlacer->queue_event(xfVideoImage2);
    }
    else
    {
	videoOutput->queue_event(xfVideoImage);
    }

    avFrameIsFree = true;
    DEBUG(<< "Queueing XFVideoImage");
}
