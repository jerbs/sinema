#include "player/VideoDecoder.hpp"
#include "player/VideoOutput.hpp"
#include "player/Demuxer.hpp"

#include <boost/make_shared.hpp>
#include <iomanip>

using namespace std;

void VideoDecoder::process(boost::shared_ptr<InitEvent> event)
{
    DEBUG();
    demuxer = event->demuxer;
    videoOutput = event->videoOutput;
}

void VideoDecoder::process(boost::shared_ptr<StartEvent> event)
{
    DEBUG();
}

void VideoDecoder::process(boost::shared_ptr<OpenVideoStreamReq> event)
{
    DEBUG(<< "streamIndex = " << event->streamIndex);

    int videoStreamIndex = event->streamIndex;
    avFormatContext = event->avFormatContext;

    if (videoStreamIndex >= 0 &&
	videoStreamIndex <= (int)avFormatContext->nb_streams)
    {
	// Get a pointer to the codec context for the stream
	avCodecContext = avFormatContext->streams[videoStreamIndex]->codec;

	if (avCodecContext->codec_type == CODEC_TYPE_VIDEO)
	{
	    boost::shared_ptr<OpenVideoOutputReq> req(new OpenVideoOutputReq());
	    req->width = avCodecContext->width;
	    req->height = avCodecContext->height;
	    videoOutput->queue_event(req);

	    avCodec = avcodec_find_decoder(avCodecContext->codec_id);
	    if (avCodec)
	    {
		int ret = avcodec_open(avCodecContext, avCodec);
		if (ret == 0)
		{
		    avStream = avFormatContext->streams[videoStreamIndex];

		    int width = avCodecContext->width;
		    int height = avCodecContext->height;
		    enum PixelFormat dstPixFmt = PIX_FMT_YUV420P;
		    swsContext = sws_getContext(width, height,            // Source Size
						avCodecContext->pix_fmt,  // Source Format
						width, height,            // Destination Size
						PixelFormat(dstPixFmt),   // Destination Format
						SWS_BICUBIC,              // Flags
						NULL, NULL, NULL);        // SwsFilter*
		    if (swsContext)
		    {
			demuxer->queue_event(boost::make_shared<OpenVideoStreamResp>(videoStreamIndex));
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

    demuxer->queue_event(boost::make_shared<OpenVideoStreamFail>(videoStreamIndex));
}

void VideoDecoder::process(boost::shared_ptr<VideoPacketEvent> event)
{
    DEBUG();
    packetQueue.push(event);
    decode();
}

void VideoDecoder::process(boost::shared_ptr<XFVideoImage> event)
{
    DEBUG();

    frameQueue.push(event);
    queue();
    decode();
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

	DEBUG(<< "Queueing ConfirmPacketEvent");
	demuxer->queue_event(boost::make_shared<ConfirmPacketEvent>());

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
		    return;
		}
	    }
	}
	else
	{
	    ERROR(<< "avcodec_decode_video failed");
	}
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

    boost::shared_ptr<XFVideoImage> xfVideoImage(frameQueue.front());
    frameQueue.pop();

    unsigned int width = xfVideoImage->width();
    unsigned int height = xfVideoImage->height();

    if (avCodecContext->width != int(width) ||
	avCodecContext->height != int(height))
    {
	ERROR(<< "Frame buffer with wrong size");

	videoOutput->queue_event(boost::make_shared<DeleteXFVideoImage>(xfVideoImage));

	boost::shared_ptr<ResizeVideoOutputReq> req(new ResizeVideoOutputReq());
	req->width = avCodecContext->width;
	req->height = avCodecContext->height;
	videoOutput->queue_event(req);

	return;
    }

    char* data = xfVideoImage->data();

    char* Y = data;
    char* V = data + width * height;
    char* U = V + width/2 * height/2;

    AVPicture avPicture;
    avPicture.data[0] = (uint8_t*)Y;
    avPicture.data[1] = (uint8_t*)U;
    avPicture.data[2] = (uint8_t*)V;
    avPicture.data[3] = 0;
    avPicture.linesize[0] = width;
    avPicture.linesize[1] = width/2;
    avPicture.linesize[2] = width/2;
    avPicture.linesize[3] = 0;

    // Convert image into YUV format:
    sws_scale(swsContext,
	      avFrame->data,            // srcSlice:  Array with pointers to planes
	      avFrame->linesize,        // srcStride: Array with strides of each plane
	      0,                        // srcSliceY: position
	      avCodecContext->height,   // srcSliceH: height of the source slice
	      avPicture.data,                // dstSlice:  Array with pointers to planes
	      avPicture.linesize);           // dstStride: Array with strides of each plane

    xfVideoImage->setPTS(pts);
    videoOutput->queue_event(xfVideoImage);
    avFrameIsFree = true;
    DEBUG(<< "Queueing XFVideoImage");
}

