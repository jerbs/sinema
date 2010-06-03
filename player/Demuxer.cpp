//
// Demultiplexer
//
// Copyright (C) Joachim Erbs, 2009
//

#include "player/Demuxer.hpp"
#include "player/AudioDecoder.hpp"
#include "player/VideoDecoder.hpp"
#include "player/MediaPlayer.hpp"

#include <boost/make_shared.hpp>
#include <stdlib.h>
#include <sys/types.h>

Demuxer* Demuxer::obj;

Demuxer::Demuxer(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      m_event_processor(evt_proc),
      avFormatContext(0),
      systemStreamStatus(SystemStreamClosed),
      systemStreamFailed(false),
      audioStreamIndex(-1),
      videoStreamIndex(-1),
      audioStreamStatus(StreamClosed),
      videoStreamStatus(StreamClosed),
      queuedAudioPackets(0),
      queuedVideoPackets(0),
      targetQueuedAudioPackets(10),
      targetQueuedVideoPackets(10)
{
    av_register_all();

    // This only works for a single Demuxer class:
    obj = this;
}

Demuxer::~Demuxer(){}

int Demuxer::interrupt_cb()
{
    return obj->m_event_processor->terminating();
}

void Demuxer::process(boost::shared_ptr<InitEvent> event)
{
    DEBUG(<< "tid = " << gettid());
    mediaPlayer = event->mediaPlayer;
    audioDecoder = event->audioDecoder;
    videoDecoder = event->videoDecoder;
}

void Demuxer::process(boost::shared_ptr<StopEvent> event)
{
    DEBUG();
}

void Demuxer::process(boost::shared_ptr<OpenFileReq> event)
{
    if (systemStreamStatus == SystemStreamClosed)
    {
	DEBUG(<< event->fileName);

	fileName = event->fileName;

	int ret;

	// Open a media file as input
	ret = av_open_input_file(&avFormatContext,
				 fileName.c_str(),
				 0,   // don't force any format, AVInputFormat*,
				 0,   // use default buffer size
				 0);  // default AVFormatParameters*
	if (ret != 0)
	{
	    ERROR(<< "av_open_input_file failed: " << ret);
	    mediaPlayer->queue_event(boost::make_shared<OpenFileFail>());
	    return;
	}

	// Read packets of a media file to get stream information
	ret = av_find_stream_info(avFormatContext);
	if (ret < 0)
	{
	    ERROR(<< "av_find_stream_info failed: " << ret);
	    av_close_input_file(avFormatContext);
	    mediaPlayer->queue_event(boost::make_shared<OpenFileFail>());
	    return;
	}

	systemStreamStatus = SystemStreamOpening;

	// Dump information about file onto standard error
	dump_format(avFormatContext, 0, event->fileName.c_str(), 0);

	// Find the first audio and video stream
	for (unsigned int i=0; i < avFormatContext->nb_streams; i++)
	{
	    if (avFormatContext->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO &&
		audioStreamIndex < 0)
	    {
		audioStreamIndex = i;
	    }

	    if (avFormatContext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO &&
		videoStreamIndex < 0)
	    {
		videoStreamIndex = i;
	    }
	}

	if (audioStreamIndex >= 0)
	{
	    audioDecoder->queue_event(boost::make_shared<OpenAudioStreamReq>(audioStreamIndex,
									     avFormatContext) );
	    audioStreamStatus = StreamOpening;
	}

	if (videoStreamIndex >= 0)
	{
	    videoDecoder->queue_event(boost::make_shared<OpenVideoStreamReq>(videoStreamIndex,
									     avFormatContext) );
	    videoStreamStatus = StreamOpening;
	}
    }
    else if (systemStreamStatus == SystemStreamOpened)
    {
	DEBUG(<< "opened");
	mediaPlayer->queue_event(boost::make_shared<OpenFileFail>());
    }
    else if (systemStreamStatus == SystemStreamClosing ||
	     systemStreamStatus == SystemStreamOpening)
    {
	// Wait until current procedure is finished.
	// Directly putting the event back into the queue
	// would result in a busy loop.
	DEBUG(<< "defer");
	defer_event(event);
    }
}

void Demuxer::process(boost::shared_ptr<OpenAudioStreamResp> event)
{
    if (audioStreamStatus == StreamOpening)
    {
	DEBUG();
	audioStreamStatus = StreamOpened;
	updateSystemStreamStatusOpening();
    }
}

void Demuxer::process(boost::shared_ptr<OpenAudioStreamFail> event)
{
    if (audioStreamStatus == StreamOpening)
    {
	DEBUG();
	mediaPlayer->queue_event(boost::make_shared<OpenAudioStreamFailed>());
	audioStreamStatus = StreamClosed;
	audioStreamIndex = -1;
	updateSystemStreamStatusOpening();
    }
}

void Demuxer::process(boost::shared_ptr<OpenVideoStreamResp> event)
{
    if (videoStreamStatus == StreamOpening)
    {
	DEBUG();
	videoStreamStatus = StreamOpened;
	updateSystemStreamStatusOpening();
    }
}

void Demuxer::process(boost::shared_ptr<OpenVideoStreamFail> event)
{
    if (videoStreamStatus == StreamOpening)
    {
	DEBUG();
	mediaPlayer->queue_event(boost::make_shared<OpenVideoStreamFailed>());
	videoStreamStatus = StreamClosed;
	videoStreamIndex = -1;
	updateSystemStreamStatusOpening();
    }
}

void Demuxer::updateSystemStreamStatusOpening()
{
    if(systemStreamStatus == SystemStreamOpening)
    {
	if ( audioStreamStatus == StreamOpening ||
	     videoStreamStatus == StreamOpening )
	{
	    // Procedures still in progress.
	    // Wait until completed.
	    return;
	}

	if ( audioStreamStatus == StreamClosed &&
	     videoStreamStatus == StreamClosed )
	{
	    // Opening audio and video stream failed.
	    av_close_input_file(avFormatContext);
	    mediaPlayer->queue_event(boost::make_shared<OpenFileFail>());
	    return;
	}

	if (videoStreamIndex == -1)
	{
	    // Opening video stream failed or system stream has no video.
	    mediaPlayer->queue_event(boost::make_shared<NoVideoStream>());
	}

	if (audioStreamIndex == -1)
	{
	    // Opening audio stream failed or system stream has no audio.
	    mediaPlayer->queue_event(boost::make_shared<NoAudioStream>());
	}

	// At least one stream is successfully opened.
	systemStreamStatus = SystemStreamOpened;
	// mediaPlayer->queue_event(boost::make_shared<OpenFileResp>());

	boost::shared_ptr<NotificationFileInfo> nfi(new NotificationFileInfo());
	const double INV_AV_TIME_BASE = double(1)/AV_TIME_BASE;
	nfi->fileName = fileName;
	nfi->duration = double(avFormatContext->duration) * INV_AV_TIME_BASE;
	nfi->file_size = avFormatContext->file_size;
	mediaPlayer->queue_event(nfi);

	mediaPlayer->queue_event(boost::make_shared<OpenFileResp>());
    }
}

void Demuxer::process(boost::shared_ptr<CloseFileReq> event)
{
    if (systemStreamStatus == SystemStreamOpened)
    {
	DEBUG();

	systemStreamStatus = SystemStreamClosing;
	systemStreamFailed = false;

	if (audioStreamStatus != StreamClosed)
	{
	    audioDecoder->queue_event(boost::make_shared<CloseAudioStreamReq>());
	    audioStreamStatus = StreamClosing;
	    audioStreamIndex = -1;
	    queuedAudioPackets = 0;
	}

	if (videoStreamStatus != StreamClosed)
	{
	    videoDecoder->queue_event(boost::make_shared<CloseVideoStreamReq>());
	    videoStreamStatus = StreamClosing;
	    videoStreamIndex = -1;
	    queuedVideoPackets = 0;
	}
    }
    else if (systemStreamStatus == SystemStreamClosed)
    {
	DEBUG(<< "closed");

	// Is allready closed. Just send a response.
	mediaPlayer->queue_event(boost::make_shared<CloseFileResp>());
    }
    else if (systemStreamStatus == SystemStreamOpening ||
	     systemStreamStatus == SystemStreamClosing)
    {
	// Wait until current procedure is finished.
	// Directly putting the event back into the queue
	// would result in a busy loop.
	DEBUG(<< "defer");
	defer_event(event);
    }
}

void Demuxer::process(boost::shared_ptr<CloseAudioStreamResp> event)
{
    if (audioStreamStatus == StreamClosing)
    {
	DEBUG();

	audioStreamStatus = StreamClosed;
	updateSystemStreamStatusClosing();
    }
}

void Demuxer::process(boost::shared_ptr<CloseVideoStreamResp> event)
{
    if (videoStreamStatus == StreamClosing)
    {
	DEBUG();

	videoStreamStatus = StreamClosed;
	updateSystemStreamStatusClosing();
    }
}

void Demuxer::updateSystemStreamStatusClosing()
{
    if (systemStreamStatus == SystemStreamClosing)
    {
	DEBUG();
	if (audioStreamStatus == StreamClosed &&
	    videoStreamStatus == StreamClosed)
	{
	    systemStreamStatus = SystemStreamClosed;

	    av_close_input_file(avFormatContext);
	    avFormatContext = 0;

	    mediaPlayer->queue_event(boost::make_shared<CloseFileResp>());

	    queue_deferred_events();
	}
    }
}

void Demuxer::process(boost::shared_ptr<SeekRelativeReq> event)
{   
    if ( systemStreamStatus==SystemStreamOpened ||
	 systemStreamStatus==SystemStreamOpening )
    {
	DEBUG();

	// Timestamps in streams are measured in frames.
	// frames = seconds * time_base (fps).
	// The default timebase for streamIndex -1 is AV_TIME_BASE = 1.000.000.
	// For other streams it is avFormatContext->streams[streamIndex]->time_base.

	int64_t seekTarget = event->seekOffset + event->displayedFramePTS * AV_TIME_BASE;
	process(boost::make_shared<SeekAbsoluteReq>(seekTarget));
    }
}

void Demuxer::process(boost::shared_ptr<SeekAbsoluteReq> event)
{
    DEBUG(<< "systemStreamStatus=" << systemStreamStatus);

    if ( systemStreamStatus==SystemStreamOpened ||
	 systemStreamStatus==SystemStreamOpening )
    {
	DEBUG();

	int streamIndex = videoStreamIndex;
	if (streamIndex == -1) streamIndex = audioStreamIndex;
	if (streamIndex == -1) return;

	AVRational time_base = avFormatContext->streams[streamIndex]->time_base;
	int64_t targetTimestamp = av_rescale_q(event->seekTarget, AV_TIME_BASE_Q, time_base);

	int seekFlags = 0;  // AVSEEK_FLAG_BACKWARD

	int ret = av_seek_frame(avFormatContext, streamIndex,
				targetTimestamp, seekFlags);
	if (ret >= 0)
	{
	    // success

	    // systemStreamFailed includes EOF.
	    systemStreamFailed = false;

	    boost::shared_ptr<FlushReq> flushReq(new FlushReq());
	    process(flushReq);
	}
	else
	{
	    ERROR(<< "av_seek_frame failed: ret=" << ret);
	}
    }
}

void Demuxer::process(boost::shared_ptr<FlushReq> event)
{
    DEBUG();
    videoDecoder->queue_event(event);
    audioDecoder->queue_event(event);
}

void Demuxer::process(boost::shared_ptr<ConfirmAudioPacketEvent> event)
{
    if (audioStreamStatus == StreamOpened)
    {
	DEBUG();
	queuedAudioPackets--;
    }
}

void Demuxer::process(boost::shared_ptr<ConfirmVideoPacketEvent> event)
{
    if (videoStreamStatus == StreamOpened)
    {
	DEBUG();
	queuedVideoPackets--;
    }
}

void Demuxer::operator()()
{
    // FFmpeg regulary calls interrupt_cb in blocking functions to test 
    // if asynchronous interruption is needed:
    url_set_interrupt_cb(interrupt_cb);

    AVPacket avPacketStorage;
    AVPacket* avPacket = &avPacketStorage;

    while(!m_event_processor->terminating())
    {
	DEBUG();

	if ( systemStreamStatus == SystemStreamOpened &&
	     !systemStreamFailed &&
	     ( ( audioStreamStatus == StreamOpened &&
		 queuedAudioPackets < targetQueuedAudioPackets ) ||
	       ( videoStreamStatus == StreamOpened &&
		 queuedVideoPackets < targetQueuedVideoPackets ) ) )
	{
	    int ret = av_read_frame(avFormatContext, avPacket);
	    if (ret == 0)
	    {
		DEBUG(<< "av_read_frame: stream_index = " << avPacket->stream_index);
		if (avPacket->stream_index == videoStreamIndex)
		{
		    DEBUG(<< "sending VideoPacketEvent");
		    queuedVideoPackets++;
		    videoDecoder->queue_event(boost::make_shared<VideoPacketEvent>(avPacket));
		}
		else if (avPacket->stream_index == audioStreamIndex)
		{
		    DEBUG(<< "sending AudioPacketEvent");
		    queuedAudioPackets++;
		    audioDecoder->queue_event(boost::make_shared<AudioPacketEvent>(avPacket));
		}
		else
		{
		    av_free_packet(avPacket);
		}
	    }
	    else
	    {
		ERROR(<< "av_read_frame failed: " << ret);

		systemStreamFailed = true;

		// Initiate closing the file by sending end of stream indications:
		mediaPlayer->queue_event(boost::make_shared<EndOfSystemStream>());
		audioDecoder->queue_event(boost::make_shared<EndOfAudioStream>());
		videoDecoder->queue_event(boost::make_shared<EndOfVideoStream>());
	    }

	    if (!m_event_processor->empty())
	    {
		m_event_processor->dequeue_and_process();
	    }
	}
	else
	{
	    m_event_processor->dequeue_and_process();
	}
    }
}
