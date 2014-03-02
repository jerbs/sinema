//
// Audio Decoder
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

#include "player/AudioDecoder.hpp"
#include "player/AudioOutput.hpp"
#include "player/AudioFrame.hpp"
#include "player/Demuxer.hpp"
#include "player/JpegWriter.hpp"

#include <boost/make_shared.hpp>
#include <iomanip>
#include <sys/types.h>
#include <algorithm>

AudioDecoder::AudioDecoder(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      state(Closed),
      avFormatContext(0),
      avCodecContext(0),
      avCodec(0),
      avStream(0),
      audioStreamIndex(-1),
      avPacketIsFree(true),
      avFrame(avcodec_alloc_frame()),
      avFrameIsFree(true),
      pts(0),
      avFrameBytesTransmittedPerLine(0),
      outputAvSampleFormat(AV_SAMPLE_FMT_NONE),
      sampleSize(0),
      eos(false)
{
}

AudioDecoder::~AudioDecoder()
{
    if (avFrame)
    {
	av_free(avFrame);
    }
}

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

	if (avCodecContext->codec_type == AVMEDIA_TYPE_AUDIO)
	{
	    avCodec = avcodec_find_decoder(avCodecContext->codec_id);
	    if (avCodec)
	    {
		if (avCodec->sample_fmts)
		{
		    AVSampleFormat fmt;
		    int i = 0;
		    while ((fmt = *(avCodec->sample_fmts + (i++))) != -1)
		    {
			TRACE_DEBUG(<< "supported sample format: " << fmt);
		    }
		}
		int ret = avcodec_open2(avCodecContext, avCodec, 0);
		if (ret == 0)
		{
		    avStream = avFormatContext->streams[audioStreamIndex];

		    bool supported = true;

		    switch (avCodecContext->sample_fmt)
		    {
		    case AV_SAMPLE_FMT_U8:  outputAvSampleFormat = AV_SAMPLE_FMT_U8; sampleSize = 1; break;
		    case AV_SAMPLE_FMT_S16: outputAvSampleFormat = AV_SAMPLE_FMT_S16; sampleSize = 2; break;
		    case AV_SAMPLE_FMT_S32: outputAvSampleFormat = AV_SAMPLE_FMT_S32; sampleSize = 4; break;
		    case AV_SAMPLE_FMT_FLT: outputAvSampleFormat = AV_SAMPLE_FMT_FLT; sampleSize = sizeof(float); break;
		    case AV_SAMPLE_FMT_DBL: outputAvSampleFormat = AV_SAMPLE_FMT_DBL; sampleSize = sizeof(double); break;
		    case AV_SAMPLE_FMT_U8P:  outputAvSampleFormat = AV_SAMPLE_FMT_U8; sampleSize = 1; break;
		    case AV_SAMPLE_FMT_S16P: outputAvSampleFormat = AV_SAMPLE_FMT_S16; sampleSize = 2; break;
		    case AV_SAMPLE_FMT_S32P: outputAvSampleFormat = AV_SAMPLE_FMT_S32; sampleSize = 4; break;
		    case AV_SAMPLE_FMT_FLTP: outputAvSampleFormat = AV_SAMPLE_FMT_FLT; sampleSize = sizeof(float); break;
		    case AV_SAMPLE_FMT_DBLP: outputAvSampleFormat = AV_SAMPLE_FMT_DBL; sampleSize = sizeof(double); break;
		    default:
			supported = false;
		    }

		    if (supported)
		    {
			boost::shared_ptr<OpenAudioOutputReq>
			    req(new OpenAudioOutputReq(avCodecContext->sample_rate,
						       avCodecContext->channels,
						       outputAvSampleFormat,
						       AVCODEC_MAX_AUDIO_FRAME_SIZE));
			audioOutput->queue_event(req);

			state = Opening;

			return;
		    }

		    TRACE_ERROR(<< "unsupported sample format: " << avCodecContext->sample_fmt);

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
	outputAvSampleFormat = AV_SAMPLE_FMT_NONE;
	sampleSize = 0;

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

	// Throw away all queued frames:
	while (!frameQueue.empty())
	{
	    frameQueue.pop();
	}
	avFrameIsFree = true;

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
	outputAvSampleFormat = AV_SAMPLE_FMT_NONE;
	sampleSize = 0;

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
	queue();
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

	avFrameIsFree = true;

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
    while (1)
    {
	if (!avFrameIsFree)
	{
	    TRACE_DEBUG(<< "frame not yet queued");
	    // Wait until current frame is transmitted to AudioOutput.
	    return;
	}

	if (packetQueue.empty())
	{
	    // Nothing to decode.
	    break;
	}

	if (avPacketIsFree)
	{
	    boost::shared_ptr<AudioPacketEvent> audioPacketEvent(packetQueue.front());
	    avPacket = audioPacketEvent->avPacket;
	    avPacketIsFree = false;
	}

	if (avPacket.size == 0)
	{
	    // Decoded complete packet.
	    packetQueue.pop();
	    avPacketIsFree = true;
	    TRACE_DEBUG(<< "Queueing ConfirmAudioPacketEvent");
	    demuxer->queue_event(boost::make_shared<ConfirmAudioPacketEvent>());
	    continue;
	}

	int frameFinished;
	int ret = avcodec_decode_audio4(avCodecContext, avFrame, &frameFinished, &avPacket);

	if (ret>=0)
	{
	    // ret contains the number of bytes consumed in packet.
	    avPacket.size -= ret;
	    avPacket.data += ret;

	    if (frameFinished)
	    {
#if 1
		int64_t int_pts = av_frame_get_best_effort_timestamp(avFrame);
#else
		int64_t int_pts = avFrame->pkt_pts;
#endif

		if (int_pts != AV_NOPTS_VALUE)
		{
		    pts = int_pts;
		    pts *= av_q2d(avStream->time_base);
		    avFrameIsFree = false;
		    avFrameBytesTransmittedPerLine = 0;

		    {
			static double lastPts = 0;

			TRACE_INFO( << "ADEC: pts=" << std::fixed << std::setprecision(3) << pts
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
	else
	{
	    // Decode failed, consume complete packet.
	    avPacket.size = 0;
	    TRACE_ERROR(<< "avcodec_decode_video failed: " << AvErrorCode(ret));
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

template<typename T>
inline void interleave(T* d, T* s, int bytesToCopy, int numChannels)
{
    while(bytesToCopy > 0)
    {
	*d = *(s++);
	bytesToCopy -= sizeof(T);
	d += numChannels;
    }
}

inline void interleave(void* dst, void* src, int bytesToCopy, int sampleSize, int numChannels)
{
    // bytes_to_copy must be a multiple of dest_offset and
    // dest_offset must be a multiple of sample_size.

    if (sampleSize == sizeof(double))
    {
	typedef double* T;
	interleave(T(dst), T(src), bytesToCopy, numChannels);
    }
    else if (sampleSize == sizeof(float))
    {
	typedef float* T;
	interleave(T(dst), T(src), bytesToCopy, numChannels);
    }
    else if (sampleSize == 4)
    {
	typedef uint32_t* T;
	interleave(T(dst), T(src), bytesToCopy, numChannels);
    }
    else if (sampleSize == 2)
    {
	typedef uint16_t* T;
	interleave(T(dst), T(src), bytesToCopy, numChannels);
    }
    else if (sampleSize == 1)
    {
	typedef uint8_t* T;
	interleave(T(dst), T(src), bytesToCopy, numChannels);

    }
    else
    {
	uint8_t* d = (uint8_t*)dst;
	uint8_t* s = (uint8_t*)src;
	int destOffset = numChannels * sampleSize;
	while(bytesToCopy > 0)
	{
	    for(int i=0; i<sampleSize; i++)
	    {
		*(d+i) = *(s++);
	    }
	    bytesToCopy -= sampleSize;
	    d += destOffset;
	}
    }
}

void AudioDecoder::queue()
{
    while(1)
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

	boost::shared_ptr<AudioFrame> audioFrame(frameQueue.front());
	frameQueue.pop();

	double durationAlreadyTransmitted =
	    double(avFrameBytesTransmittedPerLine) /
	    double(sampleSize * avCodecContext->sample_rate);
	double audioFramePTS = pts + durationAlreadyTransmitted;
	audioFrame->setPTS(audioFramePTS);

	TRACE_DEBUG(<< "CodecSampleFormat = " << avCodecContext->sample_fmt
		    << ", OutputSampleFormat = " << outputAvSampleFormat
		    << ", numChannels = " << avCodecContext->channels
		    << ", sampleSize = " << sampleSize
		    << ", avFrameBytesTransmittedPerLine = " << avFrameBytesTransmittedPerLine
		    << ", sampleRate = " << avCodecContext->sample_rate
		    << ", nb_samples = " << avFrame->nb_samples
		    << ", lineSize = " << avFrame->linesize[0] << "," << avFrame->linesize[1]
		    << ", data = " << (void*)(avFrame->data[0]) << "," << (void*)(avFrame->data[1]));

	// Note: In general avFrame->linesize[i] is not equal to avFrame->nb_samples * sampleSize.
	//       avFrame->nb_samples always contains the number of samples per channel.

	if (avCodecContext->sample_fmt != outputAvSampleFormat)
	{
	    // Convert planar format into interleaved format:
	    // For planar audio, each channel plane must be the same size.

	    int bytesToCopyPerLine = (avFrame->nb_samples * sampleSize) - avFrameBytesTransmittedPerLine;

	    if (bytesToCopyPerLine * avCodecContext->channels > audioFrame->numAllocatedBytes())
	    {
		// Not possible to copy complete frame.
		bytesToCopyPerLine = audioFrame->numAllocatedBytes() / avCodecContext->channels;
	    }

	    for (int i=0; i<avCodecContext->channels; i++)
	    {
		interleave(((uint8_t*)audioFrame->data()) + i * sampleSize,
			   avFrame->data[i] + avFrameBytesTransmittedPerLine,
			   bytesToCopyPerLine,
			   sampleSize,
			   avCodecContext->channels);
	    }

	    audioFrame->setFrameByteSize(bytesToCopyPerLine * avCodecContext->channels);
	    avFrameBytesTransmittedPerLine += bytesToCopyPerLine;

	    TRACE_DEBUG(<< "PTS = " << audioFramePTS
			<< ", bytesToCopyPerLine = " << bytesToCopyPerLine
			<< ", bufferSize = " << audioFrame->getFrameByteSize());
	}
	else
	{
	    // For interleaved format only plane 0 is set.

	    int size = std::min(audioFrame->numAllocatedBytes(),
				(avFrame->nb_samples * sampleSize) - avFrameBytesTransmittedPerLine);

	    memcpy(audioFrame->data(),
		   avFrame->data[0] + avFrameBytesTransmittedPerLine,
		   size);

	    audioFrame->setFrameByteSize(size);
	    avFrameBytesTransmittedPerLine += size;
	}

#ifdef STORE_DECODER_OUTPUT_ENABLED
	// This is for debugging AV-sync issues:
	JpegWriter::write("audio", framePTS, audioFrame.get(),
			  avCodecContext->sample_rate,
			  avCodecContext->channels,
			  avCodecContext->sample_fmt);
#endif

	{
	    double duration =
		double(audioFrame->getFrameByteSize()) /
		double(sampleSize * avCodecContext->channels * avCodecContext->sample_rate);
	    TRACE_DEBUG(<< "Queueing AudioFrame: PTS=" << audioFrame->getPTS()
			<< ", duration=" << duration
			<< ", nextPTS=" << audioFrame->getPTS() + duration
			<< ", size=" << audioFrame->getFrameByteSize() << " bytes");
	}

	audioOutput->queue_event(audioFrame);

	if (avFrameBytesTransmittedPerLine == avFrame->nb_samples * sampleSize)
	{
	    avFrameIsFree = true;
	    return;
	}
    }
}
