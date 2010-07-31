//
// Media Player
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

#include "player/MediaPlayer.hpp"
#include "player/Demuxer.hpp"
#include "player/VideoDecoder.hpp"
#include "player/AudioDecoder.hpp"
#include "player/VideoOutput.hpp"
#include "player/AudioOutput.hpp"
#include "player/Deinterlacer.hpp"
#include "player/PlayList.hpp"

#include <boost/make_shared.hpp>

extern "C"
{
#include <libavutil/log.h>
}

void logfunc(void* /* p */, int /* i */, const char* format, va_list ap)
{
    const size_t size = 1024;
    static char buffer[size];
    static int pos = 0;

    int needed = vsnprintf (&buffer[pos], size-pos, format, ap);
    if (needed < 0)
    {
	TRACE_ERROR(<< "vsnprintf failed: " << needed);
    }
    else if (needed > int(size-pos))
    {
	// Buffer is too small.

	if (pos)
	{
	    // Print the old fragment:
	    TraceUnit traceUnit;
	    traceUnit << "FFmpeg: " << buffer;
	    pos = 0;
	}

	// Print the new fragment:
	char buffer[needed];
	vsnprintf (buffer, needed, format, ap);
	if (buffer[needed-1] == '\n') buffer[needed-1] = 0;
	TraceUnit traceUnit;
	traceUnit << "FFmpeg: " << buffer;
    }
    else
    {
	while(buffer[pos] != 0)
	{
	    if (buffer[pos] == '\n')
	    {
		// buffer may contain multiple lines:
		int l = strlen(buffer);
		if (buffer[l-1] == '\n') buffer[l-1] = 0;
		TraceUnit traceUnit;
		traceUnit << "FFmpeg: " << buffer;
		pos = 0;
		break;
	    }
	    pos++;
	}
    }
}

// ===================================================================

MediaPlayerThreadNotification::MediaPlayerThreadNotification()
{
    // Here the GUI thread is notified to call MediaPlayer::processEventQueue();
    if (m_fct)
    {
	m_fct();
    }
}

void MediaPlayerThreadNotification::setCallback(fct_t fct)
{
    m_fct = fct;
}

MediaPlayerThreadNotification::fct_t MediaPlayerThreadNotification::m_fct;

// ===================================================================

MediaPlayer::MediaPlayer(PlayList& playList)
    : base_type(boost::make_shared<event_processor<
		concurrent_queue<receive_fct_t,
		MediaPlayerThreadNotification> > >()),
      m_PlayList(playList),
      hasAudioStream(false),
      hasVideoStream(false),
      endOfAudioStream(false),
      endOfVideoStream(false)
{
    av_log_set_callback(logfunc);
    av_log_set_level(AV_LOG_INFO);

    // Create event_processor instances:
    demuxerEventProcessor = boost::make_shared<event_processor<> >();
    decoderEventProcessor = boost::make_shared<event_processor<> >();
    outputEventProcessor = boost::make_shared<event_processor<> >();

    // Create event_receiver instances:
    demuxer = boost::make_shared<Demuxer>(demuxerEventProcessor);
    videoDecoder = boost::make_shared<VideoDecoder>(decoderEventProcessor);
    audioDecoder = boost::make_shared<AudioDecoder>(decoderEventProcessor);
    // Execute VideoOutput in GUI thread:
    videoOutput = boost::make_shared<VideoOutput>(get_event_processor());
    audioOutput = boost::make_shared<AudioOutput>(outputEventProcessor);
    deinterlacer = boost::make_shared<Deinterlacer>(decoderEventProcessor);

    // Start all event_processor instance except the own one in an separate thread.
    // Demuxer has a custom main loop:
    demuxerThread = boost::thread( demuxerEventProcessor->get_callable(demuxer) );
    decoderThread = boost::thread( decoderEventProcessor->get_callable() );
    outputThread  = boost::thread( outputEventProcessor->get_callable() );
}

MediaPlayer::~MediaPlayer()
{
    boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
    demuxerEventProcessor->queue_event(quitEvent);
    decoderEventProcessor->queue_event(quitEvent);
    outputEventProcessor->queue_event(quitEvent);

    demuxerThread.join();
    decoderThread.join();
    outputThread.join();
}

void MediaPlayer::init()
{
    sendInitEvents();

    // Use a Smart Pointer:
    // http://www.boost.org/doc/libs/1_39_0/libs/smart_ptr/smart_ptr.htm
    // http://www.boost.org/doc/libs/1_39_0/libs/smart_ptr/intrusive_ptr.html

    // make_shared, allocate_shared (with user-supplied allocator)
    // This should be encapsulated to allow switching between both solutions 
    // without changing user code.

    // demuxer->queue_event(new Start()); // OK: This fails to compile.
}

void MediaPlayer::sendInitEvents()
{
     boost::shared_ptr<InitEvent> initEvent(new InitEvent());

     initEvent->mediaPlayer = this;
     initEvent->demuxer = demuxer;
     initEvent->videoDecoder = videoDecoder;
     initEvent->audioDecoder = audioDecoder;
     initEvent->videoOutput = videoOutput;
     initEvent->audioOutput = audioOutput;
     initEvent->deinterlacer = deinterlacer;

     demuxer->queue_event(initEvent);
     videoDecoder->queue_event(initEvent);
     audioDecoder->queue_event(initEvent);
     videoOutput->queue_event(initEvent);
     audioOutput->queue_event(initEvent);
     deinterlacer->queue_event(initEvent);
}

void MediaPlayer::processEventQueue()
{
    while(!get_event_processor()->empty())
    {
	get_event_processor()->dequeue_and_process();
    }
}

void MediaPlayer::open()
{
    std::string file = m_PlayList.getCurrent();
    if (!file.empty())
    {
	demuxer->queue_event(boost::make_shared<OpenFileReq>(file));

	// Assume that audio and video is available in the stream:
	hasAudioStream = true;
	hasVideoStream = true;
    }
}

void MediaPlayer::close()
{
    demuxer->queue_event(boost::make_shared<CloseFileReq>());
}

void MediaPlayer::play()
{
    boost::shared_ptr<CommandPlay> commandPlay(new CommandPlay());
    videoOutput->queue_event(commandPlay);
    audioOutput->queue_event(commandPlay);
}

void MediaPlayer::pause()
{
    boost::shared_ptr<CommandPause> commandPause(new CommandPause());
    videoOutput->queue_event(commandPause);
    audioOutput->queue_event(commandPause);
}

void MediaPlayer::skipBack()
{
    if (m_PlayList.selectPrevious())
    {
	close();
	open();
    }
}

void MediaPlayer::skipForward()
{
    skipForwardInt();
}

bool MediaPlayer::skipForwardInt()
{
    if (m_PlayList.selectNext())
    {
	close();
	open();
	return true;
    }

    // No next file in play list.
    return false;
}

void MediaPlayer::seekAbsolute(double second)
{
    demuxer->queue_event(boost::make_shared<SeekAbsoluteReq>
			 (second*AV_TIME_BASE));
}

void MediaPlayer::seekRelative(double secondsDelta)
{
    TRACE_DEBUG(<< secondsDelta);
    // Send SeekRelativeReq indirectly to Demuxer via VideoOutput 
    // which has to fill the current PTS:
    videoOutput->queue_event(boost::make_shared<SeekRelativeReq>
			     (secondsDelta*AV_TIME_BASE));
}

void MediaPlayer::setPlaybackVolume(double volume)
{
    audioOutput->queue_event(boost::make_shared<CommandSetPlaybackVolume>(volume));
}

void MediaPlayer::setPlaybackSwitch(bool enabled)
{
    audioOutput->queue_event(boost::make_shared<CommandSetPlaybackSwitch>(enabled));
}

void MediaPlayer::clip(boost::shared_ptr<ClipVideoDstEvent> event)
{
    videoOutput->queue_event(event);
}

void MediaPlayer::clip(boost::shared_ptr<ClipVideoSrcEvent> event)
{
    videoOutput->queue_event(event);
}

void MediaPlayer::enableOptimalPixelFormat()
{
    videoDecoder->queue_event(boost::make_shared<EnableOptimalPixelFormat>());
}

void MediaPlayer::disableOptimalPixelFormat()
{
    videoDecoder->queue_event(boost::make_shared<DisableOptimalPixelFormat>());
}

void MediaPlayer::enableXvClipping()
{
    videoOutput->queue_event(boost::make_shared<EnableXvClipping>());
}

void MediaPlayer::disableXvClipping()
{
    videoOutput->queue_event(boost::make_shared<DisableXvClipping>());
}

void MediaPlayer::selectDeinterlacer(const std::string& name)
{
    deinterlacer->queue_event(boost::make_shared<SelectDeinterlacer>(name));
}

void MediaPlayer::process(boost::shared_ptr<OpenFileResp>)
{
    TRACE_DEBUG();
}

void MediaPlayer::process(boost::shared_ptr<OpenFileFail> event)
{
    TRACE_DEBUG(<< event->reason);

    if (event->reason == OpenFileFail::AlreadyOpened)
    {
	// Pressing play in the GUI always sends an Open and a Play event.
	// The Demuxer then also receives a OpenFileReq when Play is pressed 
	// in pause mode.
    }
    else
    {
	skipForward();
    }
}

void MediaPlayer::process(boost::shared_ptr<CloseFileResp>)
{
    TRACE_DEBUG();
}

void MediaPlayer::process(boost::shared_ptr<NoAudioStream> event)
{
    TRACE_DEBUG();
    hasAudioStream = false;
    videoOutput->queue_event(event);
}

void MediaPlayer::process(boost::shared_ptr<NoVideoStream> event)
{
    TRACE_DEBUG();
    hasVideoStream = false;
    audioOutput->queue_event(event);
}

void MediaPlayer::process(boost::shared_ptr<EndOfSystemStream>)
{
    TRACE_DEBUG();
    endOfAudioStream = false;
    endOfVideoStream = false;
}

void MediaPlayer::process(boost::shared_ptr<EndOfAudioStream>)
{
    TRACE_DEBUG();
    endOfAudioStream = true;
    if (endOfVideoStream || ! hasVideoStream)
    {
	close();
	if (!skipForwardInt())
	{
	    // skipForward only skips until the last file is selected.
	    // Application should see that the whole play list is finished:
	    m_PlayList.selectEndOfList();
	}
    }
}

void MediaPlayer::process(boost::shared_ptr<EndOfVideoStream>)
{
    TRACE_DEBUG();
    endOfVideoStream = true;
    if (endOfAudioStream || ! hasAudioStream)
    {
	close();
	if (!skipForwardInt())
	{
	    // skipForward only skips until the last file is selected.
	    // Application should see that the whole play list is finished:
	    m_PlayList.selectEndOfList();
	}
    }
}

void MediaPlayer::process(boost::shared_ptr<AudioSyncInfo> event)
{
    TRACE_DEBUG();
    // This messeage is only received for files having an audio stream only.
    process(boost::make_shared<NotificationCurrentTime>(event->pts));
}

std::ostream& operator<<(std::ostream& strm, OpenFileFail::Reason reason)
{
    switch (reason)
    {
    case OpenFileFail::OpenFileFailed:   strm << "OpenFileFailed"; break;
    case OpenFileFail::FindStreamFailed: strm << "FindStreamFailed"; break;
    case OpenFileFail::OpenStreamFailed: strm << "OpenStreamFailed"; break;
    case OpenFileFail::AlreadyOpened:    strm << "AlreadyOpened"; break;
    }

    return strm;
}
