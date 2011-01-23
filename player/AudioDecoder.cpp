//
// Audio Decoder
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

#include "player/AudioDecoder.hpp"
#include "player/AudioOutput.hpp"
#include "player/AudioFrame.hpp"
#include "player/Demuxer.hpp"

#include <boost/make_shared.hpp>
#include <iomanip>
#include <sys/types.h>

void AudioDecoder::process(boost::shared_ptr<InitEvent> event)
{
    TRACE_DEBUG(<< "tid = " << gettid());
    demuxer = event->demuxer;
    audioOutput = event->audioOutput;
}

void AudioDecoder::process(boost::shared_ptr<OpenAudioStreamReq> event)
{
    if (state == Closed)
    {
	TRACE_DEBUG(<< "streamIndex = " << event->streamIndex);

    audioStreamIndex = event->streamIndex;
    avFormatContext = event->avFormatContext;

    if (audioStreamIndex >= 0 &&
	audioStreamIndex <= (int)avFormatContext->nb_streams)
    {
	// Get a pointer to the codec context for the stream
	avCodecContext = avFormatContext->streams[audioStreamIndex]->codec;

	if (avCodecContext->codec_type == CODEC_TYPE_AUDIO)
	{
	    avCodec = avcodec_find_decoder(avCodecContext->codec_id);
	    if (avCodec)
	    {
		int ret = avcodec_open(avCodecContext, avCodec);
		if (ret == 0)
		{
		    avStream = avFormatContext->streams[audioStreamIndex];

		    boost::shared_ptr<OpenAudioOutputReq> req(new OpenAudioOutputReq());
		    req->sample_rate = avCodecContext->sample_rate;
		    req->channels = avCodecContext->channels;
		    req->sample_format = avCodecContext->sample_fmt;
		    req->frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
		    audioOutput->queue_event(req);

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
	    TRACE_ERROR(<< "expected audio stream");
	}
    }
    else
    {
	TRACE_ERROR(<< "Invalid streamIndex = " << audioStreamIndex);
    }

    avFormatContext = 0;
    avCodecContext = 0;
    avCodec = 0;
    avStream = 0;
    audioStreamIndex = -1;
    eos = false;

    }

    demuxer->queue_event(boost::make_shared<OpenAudioStreamFail>(audioStreamIndex));
}

void AudioDecoder::process(boost::shared_ptr<OpenAudioOutputResp>)
{
    if (state == Opening)
    {
	TRACE_DEBUG();

	demuxer->queue_event(boost::make_shared<OpenAudioStreamResp>(audioStreamIndex));
	state = Opened;
    }
}

void AudioDecoder::process(boost::shared_ptr<OpenAudioOutputFail>)
{
    if (state == Opening)
    {
	TRACE_DEBUG();

	avcodec_close(avCodecContext);

	avFormatContext = 0;
	avCodecContext = 0;
	avCodec = 0;
	avStream = 0;
	audioStreamIndex = -1;

	demuxer->queue_event(boost::make_shared<OpenAudioStreamFail>(audioStreamIndex));

	state = Closed;
    }
}

void AudioDecoder::process(boost::shared_ptr<CloseAudioStreamReq>)
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
	posCurrentPacket = 0;

	// Throw away all queued frames:
	while (!frameQueue.empty())
	{
	    frameQueue.pop();
	}
	numFramesCurrentPacket = 0;

	audioOutput->queue_event(boost::make_shared<CloseAudioOutputReq>());

	state = Closing;
    }
}

void AudioDecoder::process(boost::shared_ptr<CloseAudioOutputResp>)
{
    if (state == Closing)
    {
	TRACE_DEBUG();

	avFormatContext = 0;
	avCodecContext = 0;
	avCodec = 0;
	avStream = 0;
	audioStreamIndex = -1;

	demuxer->queue_event(boost::make_shared<CloseAudioStreamResp>());

	state = Closed;
    }
}

void AudioDecoder::process(boost::shared_ptr<AudioPacketEvent> event)
{
    if (state == Opened)
    {
	TRACE_DEBUG();

	eos = false;
	packetQueue.push(event);
	decode();
    }
}

void AudioDecoder::process(boost::shared_ptr<AudioFrame> event)
{
    if (state == Opened || state == Opening)
    {
	TRACE_DEBUG();

	frameQueue.push(event);
	decode();
    }
}

void AudioDecoder::process(boost::shared_ptr<FlushReq> event)
{
    if (state == Opened)
    {
	TRACE_DEBUG();

	// Flush buffers in ffmpeg decoder:
	avcodec_flush_buffers(avCodecContext);

	// Throw away everything received from the Demuxer:
	boost::shared_ptr<ConfirmAudioPacketEvent> confirm(new ConfirmAudioPacketEvent());
	while (!packetQueue.empty())
	{
	    packetQueue.pop();
	    demuxer->queue_event(confirm);
	}

	posCurrentPacket = 0;
	numFramesCurrentPacket = 0;

	// Forward event to AudioOutput:
	audioOutput->queue_event(event);
    }
}

void AudioDecoder::process(boost::shared_ptr<EndOfAudioStream>)
{
    if (state == Opened)
    {
	TRACE_DEBUG();
	eos = true;

	if (packetQueue.empty())
	{
	    // Decoded everything in this stream.
	    audioOutput->queue_event(boost::make_shared<EndOfAudioStream>());
	    eos = false;
	}
    }
}

extern std::ostream& operator<<(std::ostream& strm, AVRational r);

void AudioDecoder::decode()
{
    TRACE_DEBUG(<< "packetQueue: "  << (packetQueue.empty() ? "empty" : "has data")
		<< ", frameQueue: " << (frameQueue.empty()  ? "empty" : "has data"));

    while ( !packetQueue.empty() &&
	    !frameQueue.empty() )
    {
        boost::shared_ptr<AudioPacketEvent> audioPacketEvent(packetQueue.front());
	boost::shared_ptr<AudioFrame> audioFrame(frameQueue.front());

        AVPacket& avPacket = audioPacketEvent->avPacket;

	if (avPacket.size)
	{
	    AVPacket avpkt;
	    av_init_packet(&avpkt);
	    avpkt.data = avPacket.data+posCurrentPacket;
	    avpkt.size = avPacket.size-posCurrentPacket;

	    int16_t* samples = (int16_t*)audioFrame->data();
	    int frameByteSize = audioFrame->numAllocatedBytes();

	    int ret = avcodec_decode_audio3(avCodecContext,
					    samples, &frameByteSize,
					    &avpkt);
	    audioFrame->setFrameByteSize(frameByteSize);

	    if (ret>=0)
	    {
		double packetPTS;

		// ret contains number of bytes used from avPacket.
		// Maybe avcodec_decode_audio2 has to be called again.

		if (avPacket.pts != (int64_t)AV_NOPTS_VALUE)
		{
		    packetPTS = avPacket.pts;
		}
		else if (avPacket.dts != (int64_t)AV_NOPTS_VALUE)
		{
		    packetPTS = avPacket.dts;
		}
		else
		{
		    packetPTS = 0;
		}

		packetPTS *= av_q2d(avStream->time_base);

		double sampleRate = double(avCodecContext->sample_rate);
		double framePTS = packetPTS + numFramesCurrentPacket / sampleRate;

		int numChannels = avCodecContext->channels;
		posCurrentPacket += ret;
		numFramesCurrentPacket += frameByteSize / (2*numChannels);

		TRACE_INFO( << "ADEC: framePTS=" << framePTS
			    << ", packetPTS=" << packetPTS
			    << ", pts=" << avPacket.pts
			    << ", dts=" << avPacket.dts
			    << ", time_base=" << avStream->time_base
			    << ", finished=" << (posCurrentPacket == avPacket.size) );

		if (posCurrentPacket == avPacket.size)
		{
		    // Decoded the complete AVPacket
		    packetQueue.pop();
		    posCurrentPacket = 0;
		    TRACE_DEBUG(<< "Queueing ConfirmAudioPacketEvent");
		    demuxer->queue_event(boost::make_shared<ConfirmAudioPacketEvent>());
		}

		if (frameByteSize > 0)
		{
		    // Decoded samples are available
		    frameQueue.pop();
		    numFramesCurrentPacket = 0;
		    audioFrame->setPTS(framePTS);
		    TRACE_DEBUG(<< "Queueing AudioFrame");
		    audioOutput->queue_event(audioFrame);
		}
	    }
	    else
	    {
		// Skip packet
		packetQueue.pop();
		TRACE_DEBUG(<< "Queueing ConfirmAudioPacketEvent");
		demuxer->queue_event(boost::make_shared<ConfirmAudioPacketEvent>());
		
		TRACE_DEBUG(<< "W avcodec_decode_audio2 failed: " << ret);
	    }
	}
        else
        {
	    // Skip packet
	    packetQueue.pop();
	    TRACE_DEBUG(<< "Queueing ConfirmAudioPacketEvent");
	    demuxer->queue_event(boost::make_shared<ConfirmAudioPacketEvent>());

            TRACE_DEBUG(<< "W empty packet");
        }
    }

    if (eos && packetQueue.empty())
    {
	// Decoded everything in this stream.
	TRACE_DEBUG(<< "forwarding EndOfAudioStream");
	audioOutput->queue_event(boost::make_shared<EndOfAudioStream>());
	eos = false;
    }
}
