//
// Demultiplexer
//
// Copyright (C) Joachim Erbs, 2009
//

#include "player/Demuxer.hpp"
#include "player/FileReader.hpp"
#include "player/AudioDecoder.hpp"
#include "player/VideoDecoder.hpp"

#include <boost/make_shared.hpp>
#include <stdlib.h>

Demuxer* Demuxer::obj;

Demuxer::Demuxer(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      m_event_processor(evt_proc),
      avFormatContext(0),
      systemStreamStatus(SystemStreamClosed),
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
    DEBUG();
    fileReader = event->fileReader;
    audioDecoder = event->audioDecoder;
    videoDecoder = event->videoDecoder;
}

void Demuxer::process(boost::shared_ptr<StopEvent> event)
{
    DEBUG();
}

void Demuxer::process(boost::shared_ptr<OpenFileEvent> event)
{
    if (systemStreamStatus != SystemStreamClosed)
    {
	return;
    }

    DEBUG(<< event->fileName);

    int ret;

    // Open a media file as input
    ret = av_open_input_file(&avFormatContext,
			     event->fileName.c_str(),
			     0,   // don't force any format, AVInputFormat*,
			     0,   // use default buffer size
			     0);  // default AVFormatParameters*
    if (ret != 0)
    {
	ERROR(<< "av_open_input_file failed: " << ret);
	exit(-1);
    }

    // Read packets of a media file to get stream information
    ret = av_find_stream_info(avFormatContext);
    if (ret < 0)
    {
	ERROR(<< "av_find_stream_info failed: " << ret);
	av_close_input_file(avFormatContext);
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
	audioStreamStatus = StreamClosed;
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
	videoStreamStatus = StreamClosed;
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

	if ( audioStreamStatus == StreamOpened &&
	     videoStreamStatus == StreamOpened )
	{
	    // Successfully opened audio and video stream.
	    systemStreamStatus = SystemStreamOpened;
	}
	else
	{
	    // Opening at least one stream failed.
	    // Close everything again.
	    queue_event(boost::make_shared<CloseFileEvent>());
	}
    }
}

void Demuxer::process(boost::shared_ptr<CloseFileEvent> event)
{
    if (systemStreamStatus != SystemStreamClosed)
    {
	DEBUG();

	systemStreamStatus = SystemStreamClosing;

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
	}
    }
}

void Demuxer::process(boost::shared_ptr<SystemStreamChunkEvent> event)
{
    DEBUG();
    fileReader->queue_event(boost::make_shared<SystemStreamGetMoreDataEvent>());
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
	     audioStreamStatus == StreamOpened &&
	     videoStreamStatus == StreamOpened &&
	     ( queuedAudioPackets < targetQueuedAudioPackets ||
	       queuedVideoPackets < targetQueuedVideoPackets ) )
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
		systemStreamStatus = SystemStreamFailed;
		queue_event(boost::make_shared<CloseFileEvent>());
	    }
	}
	else
	{
	    m_event_processor->dequeue_and_process();
	}
    }
}
