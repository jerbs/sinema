#include "AudioDecoder.hpp"
#include "AudioOutput.hpp"
#include "Demuxer.hpp"

#include <boost/make_shared.hpp>
#include <iomanip>

// #undef DEBUG
// #define DEBUG(x) std::cout << __PRETTY_FUNCTION__  x << std::endl

void AudioDecoder::process(boost::shared_ptr<InitEvent> event)
{
    DEBUG();
    demuxer = event->demuxer;
    audioOutput = event->audioOutput;
}

void AudioDecoder::process(boost::shared_ptr<StartEvent> event)
{
    DEBUG();
}

void AudioDecoder::process(boost::shared_ptr<OpenAudioStreamReq> event)
{
    DEBUG(<< "streamIndex = " << event->streamIndex);

    audioStreamIndex = event->streamIndex;
    avFormatContext = event->avFormatContext;

    if (audioStreamIndex >= 0 &&
	audioStreamIndex <= (int)avFormatContext->nb_streams)
    {
	// Get a pointer to the codec context for the stream
	avCodecContext = avFormatContext->streams[audioStreamIndex]->codec;

	if (avCodecContext->codec_type == CODEC_TYPE_AUDIO)
	{
	    boost::shared_ptr<OpenAudioOutputReq> req(new OpenAudioOutputReq());
	    req->sample_rate = avCodecContext->sample_rate;
	    req->channels = avCodecContext->channels;
	    req->frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
	    audioOutput->queue_event(req);

	    avCodec = avcodec_find_decoder(avCodecContext->codec_id);
	    if (avCodec)
	    {
		int ret = avcodec_open(avCodecContext, avCodec);
		if (ret == 0)
		{
		    avStream = avFormatContext->streams[audioStreamIndex];

		    demuxer->queue_event(boost::make_shared<OpenAudioStreamResp>(audioStreamIndex));
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
	    ERROR(<< "expected audio stream");
	}
    }
    else
    {
	ERROR(<< "Invalid streamIndex = " << audioStreamIndex);
    }

    avFormatContext = 0;
    avCodecContext = 0;
    avCodec = 0;
    avStream = 0;
    audioStreamIndex = -1;

    demuxer->queue_event(boost::make_shared<OpenAudioStreamFail>(audioStreamIndex));
}

void AudioDecoder::process(boost::shared_ptr<AudioPacketEvent> event)
{
    DEBUG();
    packetQueue.push(event);
    decode();
}

void AudioDecoder::process(boost::shared_ptr<AFAudioFrame> event)
{
    DEBUG();
    frameQueue.push(event);
    decode();
}

extern std::ostream& operator<<(std::ostream& strm, AVRational r);


inline unsigned char lowerNibble(unsigned char c)
{
    return c & 0x0F;
}

inline unsigned char upperNibble(unsigned char c)
{
    return (c & 0xF0) >> 4;
}

template<typename PTR>
void hexDump(PTR data, int len)
{
    char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    // char* pos = reinterpret_cast<char*>(data);  // static cast fails to cast from unsigned to signed
    char* pos = (char*)data;
    char* end = pos + len;

    while(pos < end)
    {
	unsigned char c = *pos;
	std::cout << hex[upperNibble(c)] << hex[lowerNibble(c)];
	pos++;
    }

    std::cout << std::endl;
}

void AudioDecoder::decode()
{
    while ( !packetQueue.empty() &&
	    !frameQueue.empty() )
    {
        boost::shared_ptr<AudioPacketEvent> audioPacketEvent(packetQueue.front());
	boost::shared_ptr<AFAudioFrame> audioFrame(frameQueue.front());

        AVPacket& avPacket = audioPacketEvent->avPacket;

	if (avPacket.size)
	{
	    audio_pkt_pts = avPacket.pts;

	    int16_t* samples = (int16_t*)audioFrame->data();
	    int frameByteSize = audioFrame->numAllocatedBytes();
	    int ret = avcodec_decode_audio2(avCodecContext,
					    samples, &frameByteSize,
					    avPacket.data+posCurrentPacket,
					    avPacket.size-posCurrentPacket);
	    audioFrame->setFrameByteSize(frameByteSize);

	    if (ret>=0)
	    {
		posCurrentPacket += ret;

		if (posCurrentPacket == avPacket.size)
		{
		    // Decoded the complete AVPacket
		    packetQueue.pop();
		    demuxer->queue_event(boost::make_shared<ConfirmPacketEvent>());
		    posCurrentPacket = 0;
		}

		if (frameByteSize > 0)
		{
		    // Decoded samples are available
		    frameQueue.pop();
		    audioOutput->queue_event(audioFrame);
		}

#if 0
		// ret contains number of bytes used from avPacket.
		// Maybe avcodec_decode_audio2 has to be called again.

		if (avPacket.pts != (int64_t)AV_NOPTS_VALUE)
		{
		    pts = avPacket.pts;
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

		std::cout << "A: pts=" << std::fixed << std::setprecision(2) << pts
			  << ", pts=" << avPacket.pts
			  << ", dts=" << avPacket.dts << "(" << avPacket.dts-lastDts << ")"
			  << ", time_base=" << avStream->time_base << std::endl;

		lastDts = avPacket.dts;
#endif
	    }
	    else
	    {
		// Skip packet
		packetQueue.pop();
		demuxer->queue_event(boost::make_shared<ConfirmPacketEvent>());
		
		DEBUG(<< "W avcodec_decode_audio2 failed: " << ret);
	    }
	}
        else
        {
	    // Skip packet
	    packetQueue.pop();
	    demuxer->queue_event(boost::make_shared<ConfirmPacketEvent>());

            DEBUG(<< "W empty packet");
        }
    }
}
