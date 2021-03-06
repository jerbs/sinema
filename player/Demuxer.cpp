//
// Demultiplexer
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

#include "player/Demuxer.hpp"
#include "player/AudioDecoder.hpp"
#include "player/VideoDecoder.hpp"
#include "player/MediaPlayer.hpp"

#include <boost/make_shared.hpp>
#include <stdlib.h>
#include <sys/types.h>

extern "C"
{
#include <libavutil/mathematics.h>
}

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
      targetQueuedVideoPackets(10),
      numAnnouncedStreams(0)
{
    av_register_all();
}

Demuxer::~Demuxer(){}

int Demuxer::interruptCallback(void* ptr)
{
    // FFmpeg regulary calls interrupt_cb in blocking functions to test 
    // if asynchronous interruption is needed:
    Demuxer* obj = (Demuxer*)ptr;
    return obj->m_event_processor->terminating() || !obj->m_event_processor->empty();
}

void Demuxer::process(boost::shared_ptr<InitEvent> event)
{
    TRACE_DEBUG(<< "tid = " << gettid());
    mediaPlayer = event->mediaPlayer;
    audioDecoder = event->audioDecoder;
    videoDecoder = event->videoDecoder;
}

void Demuxer::process(boost::shared_ptr<StopEvent>)
{
    TRACE_DEBUG();
}

void Demuxer::process(boost::shared_ptr<OpenFileReq> event)
{
    if (systemStreamStatus == SystemStreamClosed)
    {
	TRACE_DEBUG(<< event->fileName);

	fileName = event->fileName;

	int ret;

	// Allocate an AVFormatContext
	avFormatContext = avformat_alloc_context();
	avFormatContext->interrupt_callback.callback = interruptCallback;
	avFormatContext->interrupt_callback.opaque = this;

	// Open a media file as input
	ret = avformat_open_input(&avFormatContext,
				  fileName.c_str(),
				  0,   // don't force any format, AVInputFormat*,
				  0);  // AVDictionary**
	if (ret < 0)
	{
	    TRACE_ERROR(<< "avformat_open_input failed: " << AvErrorCode(ret));
	    mediaPlayer->queue_event(boost::make_shared<OpenFileFail>(OpenFileFail::OpenFileFailed));
	    return;
	}

	// Read packets of a media file to get stream information
	ret = avformat_find_stream_info(avFormatContext, NULL);
	if (ret < 0)
	{
	    TRACE_ERROR(<< "avformat_find_stream_info failed: " << AvErrorCode(ret));
	    avformat_close_input(&avFormatContext);
	    mediaPlayer->queue_event(boost::make_shared<OpenFileFail>(OpenFileFail::FindStreamFailed));
	    return;
	}

	systemStreamStatus = SystemStreamOpening;

	// Dump information about file onto standard error
	av_dump_format(avFormatContext, 0, event->fileName.c_str(), 0);

	int audioStreamNum = 0;
	// Find the first audio and video stream
	for (unsigned int i=0; i < avFormatContext->nb_streams; i++)
	{
	    AVCodecContext* avCodecContext =  avFormatContext->streams[i]->codec;

	    // In general not all (subtitle) streams are known here.
	    // That's especially the case for MPEG2 PS/TS system streams.

	    if (avCodecContext->codec_type == AVMEDIA_TYPE_AUDIO &&
		audioStreamNum < 2)
		// 		audioStreamIndex < 0)
	    {
		audioStreamNum++;
		audioStreamIndex = i;
	    }

	    if (avCodecContext->codec_type == AVMEDIA_TYPE_VIDEO &&
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
	TRACE_DEBUG(<< "state SystemStreamOpened" << fileName << "-->" << event->fileName);

	mediaPlayer->queue_event(boost::make_shared<OpenFileFail>(OpenFileFail::AlreadyOpened));
    }
    else if (systemStreamStatus == SystemStreamClosing ||
	     systemStreamStatus == SystemStreamOpening)
    {
	// Wait until current procedure is finished.
	// Directly putting the event back into the queue
	// would result in a busy loop.
	TRACE_DEBUG(<< "defer");
	defer_event(event);
    }
}

void Demuxer::process(boost::shared_ptr<OpenAudioStreamResp>)
{
    if (audioStreamStatus == StreamOpening)
    {
	TRACE_DEBUG();
	audioStreamStatus = StreamOpened;
	updateSystemStreamStatusOpening();
    }
}

void Demuxer::process(boost::shared_ptr<OpenAudioStreamFail>)
{
    if (audioStreamStatus == StreamOpening)
    {
	TRACE_DEBUG();
	mediaPlayer->queue_event(boost::make_shared<OpenAudioStreamFailed>());
	audioStreamStatus = StreamClosed;
	audioStreamIndex = -1;
	updateSystemStreamStatusOpening();
    }
}

void Demuxer::process(boost::shared_ptr<OpenVideoStreamResp>)
{
    if (videoStreamStatus == StreamOpening)
    {
	TRACE_DEBUG();
	videoStreamStatus = StreamOpened;
	updateSystemStreamStatusOpening();
    }
}

void Demuxer::process(boost::shared_ptr<OpenVideoStreamFail>)
{
    if (videoStreamStatus == StreamOpening)
    {
	TRACE_DEBUG();
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
	    avformat_close_input(&avFormatContext);
	    mediaPlayer->queue_event(boost::make_shared<OpenFileFail>(OpenFileFail::OpenStreamFailed));
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
	mediaPlayer->queue_event(nfi);

	mediaPlayer->queue_event(boost::make_shared<OpenFileResp>());
    }
}

void Demuxer::process(boost::shared_ptr<CloseFileReq> event)
{
    if (systemStreamStatus == SystemStreamOpened)
    {
	TRACE_DEBUG();

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
	TRACE_DEBUG(<< "closed");

	// Is allready closed. Just send a response.
	mediaPlayer->queue_event(boost::make_shared<CloseFileResp>());
    }
    else if (systemStreamStatus == SystemStreamOpening ||
	     systemStreamStatus == SystemStreamClosing)
    {
	// Wait until current procedure is finished.
	// Directly putting the event back into the queue
	// would result in a busy loop.
	TRACE_DEBUG(<< "defer");
	defer_event(event);
    }
}

void Demuxer::process(boost::shared_ptr<CloseAudioStreamResp>)
{
    if (audioStreamStatus == StreamClosing)
    {
	TRACE_DEBUG();

	audioStreamStatus = StreamClosed;
	updateSystemStreamStatusClosing();
    }
}

void Demuxer::process(boost::shared_ptr<CloseVideoStreamResp>)
{
    if (videoStreamStatus == StreamClosing)
    {
	TRACE_DEBUG();

	videoStreamStatus = StreamClosed;
	updateSystemStreamStatusClosing();
    }
}

void Demuxer::updateSystemStreamStatusClosing()
{
    if (systemStreamStatus == SystemStreamClosing)
    {
	TRACE_DEBUG();
	if (audioStreamStatus == StreamClosed &&
	    videoStreamStatus == StreamClosed)
	{
	    systemStreamStatus = SystemStreamClosed;

	    avformat_close_input(&avFormatContext);
	    avFormatContext = 0;

	    numAnnouncedStreams = 0;

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
	TRACE_DEBUG();

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
    TRACE_DEBUG(<< "systemStreamStatus=" << systemStreamStatus);

    if ( systemStreamStatus==SystemStreamOpened ||
	 systemStreamStatus==SystemStreamOpening )
    {
	TRACE_DEBUG();

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
	    TRACE_ERROR(<< "av_seek_frame failed: " << AvErrorCode(ret));
	}
    }
}

void Demuxer::process(boost::shared_ptr<FlushReq> event)
{
    TRACE_DEBUG();
    videoDecoder->queue_event(event);
    audioDecoder->queue_event(event);
}

void Demuxer::process(boost::shared_ptr<ConfirmAudioPacketEvent>)
{
    if (audioStreamStatus == StreamOpened)
    {
	TRACE_DEBUG();
	queuedAudioPackets--;
    }
}

void Demuxer::process(boost::shared_ptr<ConfirmVideoPacketEvent>)
{
    if (videoStreamStatus == StreamOpened)
    {
	TRACE_DEBUG();
	queuedVideoPackets--;
    }
}

void Demuxer::operator()()
{
    AVPacket avPacketStorage;
    AVPacket* avPacket = &avPacketStorage;

    while(!m_event_processor->terminating())
    {
	TRACE_DEBUG();

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
		TRACE_DEBUG(<< "av_read_frame: stream_index = " << avPacket->stream_index);
		if (avPacket->stream_index == videoStreamIndex)
		{
		    TRACE_DEBUG(<< "sending VideoPacketEvent");
		    queuedVideoPackets++;
		    videoDecoder->queue_event(boost::make_shared<VideoPacketEvent>(avPacket));
		}
		else if (avPacket->stream_index == audioStreamIndex)
		{
		    TRACE_DEBUG(<< "sending AudioPacketEvent");
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
		TRACE_ERROR(<< "av_read_frame failed: " << AvErrorCode(ret));

		systemStreamFailed = true;

		// Initiate closing the file by sending end of stream indications:
		mediaPlayer->queue_event(boost::make_shared<EndOfSystemStream>());
		audioDecoder->queue_event(boost::make_shared<EndOfAudioStream>());
		videoDecoder->queue_event(boost::make_shared<EndOfVideoStream>());
	    }

	    // TODO: Avoid that first subtitles are lost here.

	    if (avFormatContext->nb_streams != numAnnouncedStreams)
	    {
		checkForNewStreams();
	    }

	    while (!m_event_processor->empty())
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

void Demuxer::checkForNewStreams()
{
    for (unsigned int i=numAnnouncedStreams; i < avFormatContext->nb_streams; i++)
    {
	numAnnouncedStreams++;
	sendAnnounceNewStream(i);


    }
}

void Demuxer::sendAnnounceNewStream(int index)
{
    AVStream *avStream = avFormatContext->streams[index];
    AVCodecContext* avCodecContext =  avStream->codec;
    AVDictionaryEntry *lang = av_dict_get(avStream->metadata, "language", NULL, 0);

    std::string language;
    if (lang)
    {
	language = std::string(lang->value);
    }

    std::string codecName(avcodec_get_name(avCodecContext->codec_id));

    char buf[256];
    avcodec_string(buf, sizeof(buf), avCodecContext, 0);
    std::string info(buf);

    NotificationNewStream::StreamType streamType;

    switch (avCodecContext->codec_type)
    {
    case AVMEDIA_TYPE_VIDEO:    streamType = NotificationNewStream::Video; break;
    case AVMEDIA_TYPE_AUDIO:    streamType = NotificationNewStream::Audio; break;
    case AVMEDIA_TYPE_SUBTITLE: streamType = NotificationNewStream::Subtitle; break;
    default:                    streamType = NotificationNewStream::Other; break;
    }

    std::cout << "[" << index << "] " << avCodecContext->codec_type
	      << " (" << language << ") " << codecName << " - " << info << std::endl;

    mediaPlayer->queue_event(boost::make_shared<NotificationNewStream>(index, streamType, language, info));
}

std::ostream& operator<<(std::ostream& os, const AVMediaType avMediaType)
{
    switch(avMediaType)
    {
    case AVMEDIA_TYPE_UNKNOWN: os << "Unknown"; break;
    case AVMEDIA_TYPE_VIDEO: os << "Video"; break;
    case AVMEDIA_TYPE_AUDIO: os << "Audio"; break;
    case AVMEDIA_TYPE_DATA: os << "Data"; break;
    case AVMEDIA_TYPE_SUBTITLE: os << "Subtitle"; break;
    case AVMEDIA_TYPE_ATTACHMENT: os << "Attachment"; break;
    case AVMEDIA_TYPE_NB: os << "NB"; break;
    default: os << "type=" << int(avMediaType);
    }

    return os;
}
