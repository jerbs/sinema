#include "AudioDecoder.hpp"
#include "Demuxer.hpp"

#include <boost/make_shared.hpp>

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

	    // wanted_spec.format = AUDIO_S16SYS;
	    // wanted_spec.silence = 0;
	    // wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
	    // wanted_spec.callback = audio_callback;
	    // wanted_spec.userdata = is;
	    
	    // if (SDL_OpenAudio(&wanted_spec, &spec) < 0)
	    // {
	    //   fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
	    //   return;
	    // }
	    // is->audio_hw_buf_size = spec.size;

	    avCodec = avcodec_find_decoder(avCodecContext->codec_id);
	    if (avCodec)
	    {
		int ret = avcodec_open(avCodecContext, avCodec);
		if (ret == 0)
		{
		    avStream = avFormatContext->streams[audioStreamIndex];

		    // is->audio_buf_size = 0;
		    // is->audio_buf_index = 0;

		    /* averaging filter for audio sync */
		    // is->audio_diff_avg_coef = exp(log(0.01 / AUDIO_DIFF_AVG_NB));
		    // is->audio_diff_avg_count = 0;
		    /* Correct audio only if larger error than this */
		    // is->audio_diff_threshold = 2.0 * SDL_AUDIO_BUFFER_SIZE / codecCtx->sample_rate;

		    // memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));
		    // is->audioq = new PacketQueue();
		    // SDL_PauseAudio(0);

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
    demuxer->queue_event(boost::make_shared<ConfirmPacketEvent>());
}
