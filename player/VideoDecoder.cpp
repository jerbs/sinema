//
// Video Decoder
//
// Copyright (C) Joachim Erbs, 2009-2012
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

#include "player/VideoDecoder.hpp"
#include "player/VideoOutput.hpp"
#include "player/Demuxer.hpp"
#include "player/MediaPlayer.hpp"
#include "player/Deinterlacer.hpp"
#include "player/XlibFacade.hpp"
#include "player/XlibHelpers.hpp"
#include "player/JpegWriter.hpp"

#include <boost/make_shared.hpp>
#include <iomanip>
#include <sys/types.h>

using namespace std;

VideoDecoder::VideoDecoder(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      state(Closed),
      avFormatContext(0),
      avCodecContext(0),
      avCodec(0),
      avStream(0),
      videoStreamIndex(-1),
      avPacketIsFree(true),
      avFrame(avcodec_alloc_frame()),
      avFrameIsFree(true),
      pts(0),
      m_fourccFormat(0),
      m_dstWidth(0),
      m_dstHeight(0),
      eos(false),
      swsContext(0),
      m_topFieldFirst(true),
      m_useOptimumImageFormat(true)
{
    TRACE_DEBUG(<< "tid = " << gettid());
}

VideoDecoder::~VideoDecoder()
{
    TRACE_DEBUG(<< "tid = " << gettid());

    if (avFrame)
    {
	av_free(avFrame);
    }
    
    if (swsContext)
    {
	sws_freeContext(swsContext);
    }
}

void VideoDecoder::process(boost::shared_ptr<InitEvent> event)
{
    TRACE_DEBUG(<< "tid = " << gettid());
    mediaPlayer = event->mediaPlayer;
    demuxer = event->demuxer;
    videoOutput = event->videoOutput;
    deinterlacer = event->deinterlacer;
}

static int getFourccFormat(enum PixelFormat pfm)
{
    // getFormatId must return a FOURCC format id supported by the video driver.
    // The parameter pfm is the pixel format of the frames emitted by the FFmpeg Decoder.
    // The goal is to determine a destination format the software scaler can convert
    // to with minimal overhead.

    switch(pfm)
    {
    case PIX_FMT_YUV420P: return GUID_YUV12_PLANAR;
    case PIX_FMT_YUYV422: return GUID_YUY2_PACKED;
    default:              return GUID_YUY2_PACKED;
    }
}

static enum PixelFormat getFFmpegFormat(int fid)
{
    // getFFmpegFormat converts the FOURCC format id returned by the getFormatId function
    // back into the FFmpeg pixel format id for the same image format. The returned
    // FFmpeg pixel format id is needed as distination format parameter to setup the 
    // software scaler.

    switch(fid)
    {
    case GUID_YUV12_PLANAR: return PIX_FMT_YUV420P;
    case GUID_YUY2_PACKED: return PIX_FMT_YUYV422;
    default: TRACE_THROW(std::string, << "Unsupported format id: " << fid);
    }
    return PIX_FMT_NONE;
}

void VideoDecoder::process(boost::shared_ptr<OpenVideoStreamReq> event)
{
    if (state == Closed)
    {
	TRACE_DEBUG(<< "streamIndex = " << event->streamIndex);

	videoStreamIndex = event->streamIndex;
	avFormatContext = event->avFormatContext;

	if (videoStreamIndex >= 0 &&
	    videoStreamIndex <= (int)avFormatContext->nb_streams)
	{
	    // Get a pointer to the codec context for the stream
	    avCodecContext = avFormatContext->streams[videoStreamIndex]->codec;

	    if (avCodecContext->codec_type == AVMEDIA_TYPE_VIDEO)
	    {
		avCodec = avcodec_find_decoder(avCodecContext->codec_id);
		if (avCodec)
		{
		    int ret = avcodec_open2(avCodecContext, avCodec, 0);
		    if (ret == 0)
		    {
			avStream = avFormatContext->streams[videoStreamIndex];
			int w = avCodecContext->width;
			int h = avCodecContext->height;
			AVRational& par = avCodecContext->sample_aspect_ratio;
			int pn = par.num;
			int pd = par.den;
			TRACE_DEBUG( << "size = " << w << ":" << h);
			TRACE_DEBUG( << "par  = " << pn << ":" << pd);
			if (pn == 0 || pd == 0) {pn = pd = 1;}
			if (m_useOptimumImageFormat)
			{
			    int fourccFormat = getFourccFormat(avCodecContext->pix_fmt);
			    setFourccFormat(fourccFormat);
			}
			else
			{
			    // Set format needed to use the deinterlacer:
			    setFourccFormat(GUID_YUY2_PACKED);
			}
			videoOutput->queue_event(boost::make_shared<OpenVideoOutputReq>(w,h,pn,pd,
											m_fourccFormat));

			state = Opening;

			return;
		    }
		    else
		    {
			TRACE_ERROR(<< "avcodec_open failed: " << AvErrorCode(ret));
		    }
		}
		else
		{
		    TRACE_ERROR(<< "avcodec_find_decoder failed");
		}
	    }
	    else
	    {
		TRACE_ERROR(<< "expected video stream");
	    }
	}
	else
	{
	    TRACE_ERROR(<< "Invalid streamIndex = " << videoStreamIndex);
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

void VideoDecoder::process(boost::shared_ptr<OpenVideoOutputResp>)
{
    if (state == Opening)
    {
	TRACE_DEBUG();

	demuxer->queue_event(boost::make_shared<OpenVideoStreamResp>(videoStreamIndex));
	state = Opened;
    }
}

void VideoDecoder::process(boost::shared_ptr<OpenVideoOutputFail>)
{
    if (state == Opening)
    {
	TRACE_DEBUG();

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

void VideoDecoder::process(boost::shared_ptr<CloseVideoStreamReq>)
{
    if (state != Closed)
    {
	TRACE_DEBUG();

	if (state == Opened)
	{
	    avcodec_close(avCodecContext);
	}

	// Throw away all queued packets:
	while (!packetQueue.empty())
	{
	    packetQueue.pop();
	}

	avPacketIsFree = true;

	// Throw away all queued frames:
	while (!frameQueue.empty())
	{
	    std::unique_ptr<XFVideoImage> xfVideoImage(std::move(frameQueue.front()));
	    frameQueue.pop();
	    videoOutput->queue_event(std::move(std::unique_ptr<DeleteXFVideoImage>(new DeleteXFVideoImage(std::move(xfVideoImage)))));
	}
	avFrameIsFree = true;

	// Do the same as the Deinterlacer when receiving CloseVideoOutputReq:
	m_topFieldFirst = true;

	// Do not reset m_useOptimumImageFormat, it is a configuration parameter.

	// Send event via Deinterlacer to VideoOutput:
	deinterlacer->queue_event(boost::make_shared<CloseVideoOutputReq>());

	state = Closing;
    }
}

void VideoDecoder::process(boost::shared_ptr<CloseVideoOutputResp>)
{
    if (state == Closing)
    {
	TRACE_DEBUG();

	avFormatContext = 0;
	avCodecContext = 0;
	avCodec = 0;
	avStream = 0;
	videoStreamIndex = -1;

	avPacketIsFree = true;
	// Keep avFrame.
	avFrameIsFree = true;
	pts = 0;
	m_fourccFormat = 0;

	if (swsContext)
	{
	    sws_freeContext(swsContext);
	    swsContext = 0;
	}

	m_dstWidth = 0;
	m_dstHeight = 0;

	demuxer->queue_event(boost::make_shared<CloseVideoStreamResp>());

	state = Closed;
    }
}

void VideoDecoder::process(boost::shared_ptr<VideoPacketEvent> event)
{
    if (state == Opened)
    {
	TRACE_DEBUG();

	eos = false;
	packetQueue.push(event);
	decode();
    }
}

void VideoDecoder::process(std::unique_ptr<XFVideoImage> event)
{
    if (state == Opened || state == Opening)
    {
	TRACE_DEBUG();

	int fourccFormat = event->xvImage()->id;

	// A bigger frame may have been provided.
	if (avCodecContext->width != event->requestedWidth() ||
	    avCodecContext->height != event->requestedHeight() ||
	    m_fourccFormat != fourccFormat)
	{
	    TRACE_DEBUG(<< "Frame buffer with wrong size/format."
			<< " needed:" << avCodecContext->width << "*" << avCodecContext->height
			<< std::hex << ", 0x" << m_fourccFormat
			<< std::dec <<
			" requested: " << event->requestedWidth() << "*" << event->requestedHeight()
			<< " got: " << event->width() << "*" << event->height()
			<< std::hex << ", 0x" << fourccFormat );

	    videoOutput->queue_event(std::move(std::unique_ptr<DeleteXFVideoImage>(new DeleteXFVideoImage(std::move(event)))));

	    requestNewFrame();

	    return;
	}
	else if (m_dstWidth != int(event->width()) ||
		 m_dstHeight != int(event->height()))
	{
	    m_dstWidth = event->width();
	    m_dstHeight = event->height();
	    getSwsContext();
	}

	// Add XFVideoImage with correct size and format to frameQueue:
	frameQueue.push(std::move(event));
	queue();
	decode();
    }
    else
    {
      	videoOutput->queue_event(std::move(std::unique_ptr<DeleteXFVideoImage>(new DeleteXFVideoImage(std::move(event)))));
    }
}

void VideoDecoder::process(boost::shared_ptr<FlushReq> event)
{
    if (state == Opened)
    {
	TRACE_DEBUG();

	// Flush buffers in ffmpeg decoder:
	avcodec_flush_buffers(avCodecContext);

	// Throw away everything received from the Demuxer:
	boost::shared_ptr<ConfirmVideoPacketEvent> confirm(new ConfirmVideoPacketEvent());
	while (!packetQueue.empty())
	{
	    packetQueue.pop();
	    demuxer->queue_event(confirm);
	}

	avPacketIsFree = true;
	avFrameIsFree = true;

	// Forward event via Deinterlacer to VideoOutput:
	deinterlacer->queue_event(event);
    }
}

void VideoDecoder::process(boost::shared_ptr<EndOfVideoStream> event)
{
    if (state == Opened)
    {
	TRACE_DEBUG();
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

void VideoDecoder::process(boost::shared_ptr<EnableOptimalPixelFormat>)
{
    TRACE_DEBUG();
    if (!m_useOptimumImageFormat)
    {
	m_useOptimumImageFormat = true;
	if (avCodecContext)
	{
	    // Set image format emitted by the video decoder:
	    int fourccFormat = getFourccFormat(avCodecContext->pix_fmt);
	    setFourccFormat(fourccFormat);
	}
    }
}

void VideoDecoder::process(boost::shared_ptr<DisableOptimalPixelFormat>)
{
    TRACE_DEBUG();
    if (m_useOptimumImageFormat)
    {
	m_useOptimumImageFormat = false;
	if (avCodecContext)
	{
	    // Set initial image format.
	    // This is the format needed by the deinterlacer:
	    setFourccFormat(GUID_YUY2_PACKED);
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
    while (1)
    {
	if (!avFrameIsFree)
	{
	    TRACE_DEBUG(<< "frame not yet queued");
	    // Wait until current frame is transmitted to VideoOutput.
	    return;
	}

	if (packetQueue.empty())
	{
	    // Nothing to decode.
	    break;
	}

	if (avPacketIsFree)
	{
	    boost::shared_ptr<VideoPacketEvent> videoPacketEvent(packetQueue.front());
	    avPacket = videoPacketEvent->avPacket;
	    avPacketIsFree = false;
	}

	if (avPacket.size == 0)
	{
	    // Decoded complete packet.
	    packetQueue.pop();
	    avPacketIsFree = true;
	    TRACE_DEBUG(<< "Queueing ConfirmVideoPacketEvent");
	    demuxer->queue_event(boost::make_shared<ConfirmVideoPacketEvent>());
	    continue;
	}

	int frameFinished;
	int ret = avcodec_decode_video2(avCodecContext, avFrame, &frameFinished, &avPacket);

	if (ret>0)
	{
	    // ret contains the number of bytes consumed in packet.
	    avPacket.size -= ret;
	    avPacket.data += ret;

	    if (frameFinished)
	    {
		int64_t int_pts = av_frame_get_best_effort_timestamp(avFrame);

		if (int_pts != AV_NOPTS_VALUE)
		{
		    pts = int_pts;
		    pts *= av_q2d(avStream->time_base);
		    avFrameIsFree = false;

		    {
			static double lastPts = 0;

			TRACE_INFO( << "VDEC: pts=" << std::fixed << std::setprecision(3) << pts
				    << "(" << pts - lastPts << ")"
				    << ", pts=" << avFrame->pts
				    << ", dts=" << avPacket.dts
				    << ", time_base=" << avStream->time_base
				    << ", frameFinished=" << frameFinished );

			lastPts = pts;
		    }

		    queue();

		    if (!avFrameIsFree)
		    {
			// Call queue() again when a frame is available.
			return;
		    }
		}
	    }
	}
	else if (ret == 0)
	{
	    // Decoder consumed no data. Discarding packet to avoid deadlock.
	    avPacket.size = 0;
	    TRACE_ERROR(<< "avcodec_decode_video2 returned 0");
	}
	else
	{
	    // Decoding failed, consuming complete packet.
	    avPacket.size = 0;
	    TRACE_ERROR(<< "avcodec_decode_video2 failed: " << AvErrorCode(ret));
	}
    }

    if (eos && avFrameIsFree && packetQueue.empty())
    {
	// Decoded everything in this stream.

	// Forward event via Deinterlacer to VideoOutput:
	TRACE_DEBUG(<< "forwarding EndOfVideoStream");
	deinterlacer->queue_event(boost::make_shared<EndOfVideoStream>());
	eos = false;
    }

}

void VideoDecoder::queue()
{
    if (avFrameIsFree)
    {
	// Nothing to queue
	TRACE_DEBUG(<< "nothing to queue");
	return;
    }

    if (frameQueue.empty())
    {
	TRACE_DEBUG(<< "no frames available");
	return;
    }

    if (avFrame->interlaced_frame)
    {
	if (frameQueue.size() < 2)
	{
	    return;
	}

	if (m_fourccFormat != GUID_YUY2_PACKED)
	{
	    // The deinterlacer needs the packed YUY2 format.
	    // This format is not the default.
	    setFourccFormat(GUID_YUY2_PACKED);

	    return;
	}
    }

    std::unique_ptr<XFVideoImage> xfVideoImage(std::move(frameQueue.front()));
    frameQueue.pop();

    TRACE_DEBUG( << "interlaced_frame = " << avFrame->interlaced_frame
		 << ", top_field_first = " << avFrame->top_field_first);

    XvImage* yuvImage = xfVideoImage->xvImage();
    char* data = yuvImage->data;

    AVPicture avPicture;

    TRACE_DEBUG(<< "yuvImage->id = " << std::hex << yuvImage->id);

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
	TRACE_THROW(std::string, << "unsupported format 0x" << std::hex << yuvImage->id);
	return;
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

#ifdef STORE_DECODER_OUTPUT_ENABLED
    // This is for debugging AV-sync issues:
    JpegWriter::write("video", pts, avFrame);
#endif

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

	    m_topFieldFirst = tff;
	}

	// The deinterlacer needs two frames.
	TRACE_DEBUG(<< "Queueing XFVideoImage");
	deinterlacer->queue_event(std::move(xfVideoImage));

	std::unique_ptr<XFVideoImage> xfVideoImage2(std::move(frameQueue.front()));
	frameQueue.pop();

	deinterlacer->queue_event(std::move(xfVideoImage2));
    }
    else
    {
	TRACE_DEBUG(<< "Queueing XFVideoImage");
	videoOutput->queue_event(std::move(xfVideoImage));
    }

    avFrameIsFree = true;
}

void VideoDecoder::setFourccFormat(int fourccFormat)
{
    if (m_fourccFormat != fourccFormat)
    {
	TRACE_DEBUG(<< std::hex << fourccFormat);

	m_fourccFormat = fourccFormat;

	getSwsContext();
    }
}

void VideoDecoder::getSwsContext()
{
    if (swsContext)
    {
	sws_freeContext(swsContext);
    }

    // Throw away all queued frames:
    while(!frameQueue.empty())
    {
	std::unique_ptr<XFVideoImage> xfVideoImage(std::move(frameQueue.front()));
	frameQueue.pop();
	videoOutput->queue_event(std::unique_ptr<DeleteXFVideoImage>(new DeleteXFVideoImage(std::move(xfVideoImage))));

	requestNewFrame();
    }

    if (!m_dstWidth ||
	!m_dstHeight)
    {
	// Wait until destination size is known.
	return;
    }

    enum PixelFormat dstPixFmt = getFFmpegFormat(m_fourccFormat);

    int srcWidth = avCodecContext->width;
    int srcHeight = avCodecContext->height;

    int flags =
	SWS_BICUBIC |
	SWS_CPU_CAPS_MMX | SWS_CPU_CAPS_MMX2 | // SWS_CPU_CAPS_3DNOW |
	SWS_PRINT_INFO;

    swsContext = sws_getContext(srcWidth, srcHeight,      // Source Size
				avCodecContext->pix_fmt,  // Source Format
				m_dstWidth, m_dstHeight,  // Destination Size
				dstPixFmt,                // Destination Format
				flags,                    // Flags
				NULL, NULL, NULL);        // SwsFilter*
    if (!swsContext)
    {
	TRACE_THROW(std::string, << "sws_getContext failed");
    }
}

void VideoDecoder::requestNewFrame()
{
    int w = avCodecContext->width;
    int h = avCodecContext->height;
    AVRational& par = avCodecContext->sample_aspect_ratio;
    int pn = par.num;
    int pd = par.den;
    if (pn == 0 || pd == 0) {pn = pd = 1;}

    videoOutput->queue_event(boost::make_shared<ResizeVideoOutputReq>(w,h,pn,pd,m_fourccFormat));
}

// -------------------------------------------------------------------
