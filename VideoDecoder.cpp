#include "VideoDecoder.hpp"
#include "Demuxer.hpp"

#include <boost/make_shared.hpp>

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

	    avCodec = avcodec_find_decoder(avCodecContext->codec_id);
	    if (avCodec)
	    {
		int ret = avcodec_open(avCodecContext, avCodec);
		if (ret == 0)
		{
		    avStream = avFormatContext->streams[videoStreamIndex];

		    demuxer->queue_event(boost::make_shared<OpenVideoStreamResp>(videoStreamIndex));
		    return;
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
    demuxer->queue_event(boost::make_shared<ConfirmPacketEvent>());
}
