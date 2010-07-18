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

VideoDecoder::VideoDecoder(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      state(Closed),
      avFormatContext(0),
      avCodecContext(0),
      avCodec(0),
      avStream(0),
      videoStreamIndex(-1),
      video_pkt_pts(AV_NOPTS_VALUE),
      avFrame(avcodec_alloc_frame()),
      avFrameIsFree(true),
      pts(0),
      m_imageFormat(0),
      eos(false),
      swsContext(0),
      m_topFieldFirst(true),
      m_useOptimumImageFormat(true)
{}

VideoDecoder::~VideoDecoder()
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

void VideoDecoder::process(boost::shared_ptr<InitEvent> event)
{
    TRACE_DEBUG(<< "tid = " << gettid());
    mediaPlayer = event->mediaPlayer;
    demuxer = event->demuxer;
    videoOutput = event->videoOutput;
    deinterlacer = event->deinterlacer;
}

static int getFormatId(enum PixelFormat pfm)
{
    switch(pfm)
    {
    case PIX_FMT_YUV420P: return GUID_YUV12_PLANAR;
    case PIX_FMT_YUYV422: return GUID_YUY2_PACKED;
    default: TRACE_THROW(std::string, << "Unsupported FFmpeg pixel format: " << pfm);
    }
    return 0;
}

static enum PixelFormat getFFmpegFormat(int fid)
{
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

	    if (avCodecContext->codec_type == CODEC_TYPE_VIDEO)
	    {
		avCodec = avcodec_find_decoder(avCodecContext->codec_id);
		if (avCodec)
		{
		    int ret = avcodec_open(avCodecContext, avCodec);
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
			    int formatId = getFormatId(avCodecContext->pix_fmt);
			    setImageFormat(formatId);
			}
			else
			{
			    // Set format needed to use the deinterlacer:
			    setImageFormat(GUID_YUY2_PACKED);
			}
			videoOutput->queue_event(boost::make_shared<OpenVideoOutputReq>(w,h,pn,pd,
											m_imageFormat));

			state = Opening;

			return;
		    }
		    else
		    {
			TRACE_ERROR(<< "avcodec_open failed: ret = " << ret);
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

	// Throw away all queued frames:
	while (!frameQueue.empty())
	{
	    boost::shared_ptr<XFVideoImage> xfVideoImage(frameQueue.front());
	    frameQueue.pop();
	    videoOutput->queue_event(boost::make_shared<DeleteXFVideoImage>(xfVideoImage));
	}

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

	video_pkt_pts = AV_NOPTS_VALUE;

	// Keep avFrame.
	avFrameIsFree = true;
	pts = 0;
	m_imageFormat = 0;

	if (swsContext)
	{
	    sws_freeContext(swsContext);
	    swsContext = 0;
	}

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

void VideoDecoder::process(boost::shared_ptr<XFVideoImage> event)
{
    if (state == Opened || state == Opening)
    {
	TRACE_DEBUG();

	unsigned int width = event->width();
	unsigned int height = event->height();
	int imageFormat = event->xvImage()->id;

	if (avCodecContext->width != int(width) ||
	    avCodecContext->height != int(height) ||
	    m_imageFormat != imageFormat)
	{
	    TRACE_DEBUG(<< "Frame buffer with wrong size/format."
			<< " needed:"
			<< avCodecContext->width << "*" << avCodecContext->height
			<< std::hex << ", 0x" << m_imageFormat
			<< std::dec << " got: " << width << "*" << height
			<< std::hex << ", 0x" << imageFormat );

	    videoOutput->queue_event(boost::make_shared<DeleteXFVideoImage>(event));

	    requestNewFrame();

	    return;
	}

	// Add XFVideoImage with correct size and format to frameQueue:
	frameQueue.push(event);
	queue();
	decode();
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
	    int formatId = getFormatId(avCodecContext->pix_fmt);
	    setImageFormat(formatId);
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
	    setImageFormat(GUID_YUY2_PACKED);
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
	TRACE_DEBUG(<< "frame not yet queued");
	// Wait until current frame is transmitted to VideoOutput.
	return;
    }

    while (!packetQueue.empty())
    {
	boost::shared_ptr<VideoPacketEvent> videoPacketEvent(packetQueue.front());
	packetQueue.pop();

	TRACE_DEBUG(<< "Queueing ConfirmVideoPacketEvent");
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
		*(int64_t*)avFrame->opaque != (int64_t)AV_NOPTS_VALUE)
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

	    TRACE_INFO( << "VDEC: pts=" << std::fixed << std::setprecision(2) << pts 
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
	    TRACE_ERROR(<< "avcodec_decode_video failed");
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
	TRACE_DEBUG(<< "frameQueue.empty() returned true");
	return;
    }

    if (avFrame->interlaced_frame)
    {
	if (frameQueue.size() < 2)
	{
	    return;
	}

	if (m_imageFormat != GUID_YUY2_PACKED)
	{
	    // The deinterlacer needs the packed YUY2 format.
	    // This format is not the default.
	    setImageFormat(GUID_YUY2_PACKED);

	    return;
	}
    }

    boost::shared_ptr<XFVideoImage> xfVideoImage(frameQueue.front());
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
	deinterlacer->queue_event(xfVideoImage);

	boost::shared_ptr<XFVideoImage> xfVideoImage2(frameQueue.front());
	frameQueue.pop();

	deinterlacer->queue_event(xfVideoImage2);
    }
    else
    {
	videoOutput->queue_event(xfVideoImage);
    }

    avFrameIsFree = true;
    TRACE_DEBUG(<< "Queueing XFVideoImage");
}

void VideoDecoder::setImageFormat(int imageFormat)
{
    if (m_imageFormat != imageFormat)
    {
	TRACE_DEBUG(<< std::hex << imageFormat);

	m_imageFormat = imageFormat;

	getSwsContext();

	// Throw away all queued frames:
	while(!frameQueue.empty())
	{
	    boost::shared_ptr<XFVideoImage> xfVideoImage(frameQueue.front());
	    frameQueue.pop();
	    videoOutput->queue_event(boost::make_shared<DeleteXFVideoImage>(xfVideoImage));

	    requestNewFrame();
	}
    }
}

void VideoDecoder::getSwsContext()
{
    if (swsContext)
    {
	sws_freeContext(swsContext);
    }

    enum PixelFormat dstPixFmt = getFFmpegFormat(m_imageFormat);

    int width = avCodecContext->width;
    int height = avCodecContext->height;

    int flags =
	SWS_BICUBIC |
	SWS_CPU_CAPS_MMX | SWS_CPU_CAPS_MMX2 | // SWS_CPU_CAPS_3DNOW |
	SWS_PRINT_INFO;

    swsContext = sws_getContext(width, height,            // Source Size
				avCodecContext->pix_fmt,  // Source Format
				width, height,            // Destination Size
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

    videoOutput->queue_event(boost::make_shared<ResizeVideoOutputReq>(w,h,pn,pd,m_imageFormat));
}

// -------------------------------------------------------------------
