//
// Media Player
//
// Copyright (C) Joachim Erbs, 2009
//

#include "player/MediaPlayer.hpp"

#include <boost/make_shared.hpp>

extern "C"
{
#include <libavutil/log.h>
}

void logfunc(void* p, int i, const char* format, va_list ap)
{
    printf("FFmpeg: ");
    vprintf(format, ap);
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

MediaPlayer::MediaPlayer(boost::shared_ptr<PlayList> playList)
    : base_type(boost::make_shared<event_processor<
		concurrent_queue<receive_fct_t,
		MediaPlayerThreadNotification> > >()),
      playList(playList),
      endOfAudioStream(false),
      endOfVideoStream(false)
{
    av_log_set_callback(logfunc);

    // Create event_processor instances:
    demuxerEventProcessor = boost::make_shared<event_processor<> >();
    decoderEventProcessor = boost::make_shared<event_processor<> >();
    outputEventProcessor = boost::make_shared<event_processor<> >();

    // Create event_receiver instances:
    fileReader = boost::make_shared<FileReader>(demuxerEventProcessor);
    demuxer = boost::make_shared<Demuxer>(demuxerEventProcessor);
    videoDecoder = boost::make_shared<VideoDecoder>(decoderEventProcessor);
    audioDecoder = boost::make_shared<AudioDecoder>(decoderEventProcessor);
    // Execute VideoOutput in GUI thread:
    videoOutput = boost::make_shared<VideoOutput>(get_event_processor());
    audioOutput = boost::make_shared<AudioOutput>(outputEventProcessor);

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
     initEvent->fileReader = fileReader;
     initEvent->demuxer = demuxer;
     initEvent->videoDecoder = videoDecoder;
     initEvent->audioDecoder = audioDecoder;
     initEvent->videoOutput = videoOutput;
     initEvent->audioOutput = audioOutput;

     fileReader->queue_event(initEvent);
     demuxer->queue_event(initEvent);
     videoDecoder->queue_event(initEvent);
     audioDecoder->queue_event(initEvent);
     videoOutput->queue_event(initEvent);
     audioOutput->queue_event(initEvent);
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
    std::string file = playList->getCurrent();
    if (!file.empty())
    {
	demuxer->queue_event(boost::make_shared<OpenFileReq>(file));
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
    if (playList->selectPrevious())
    {
	close();
	open();
    }
}

void MediaPlayer::skipForward()
{
    if (playList->selectNext())
    {
	close();
	open();
    }
}

void MediaPlayer::seekAbsolute(double second)
{
    demuxer->queue_event(boost::make_shared<SeekAbsoluteReq>
			 (second*AV_TIME_BASE));
}

void MediaPlayer::seekRelative(double secondsDelta)
{
    DEBUG(<< secondsDelta);
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

void MediaPlayer::process(boost::shared_ptr<OpenFileResp> event)
{
}

void MediaPlayer::process(boost::shared_ptr<OpenFileFail> event)
{
    skipForward();
}

void MediaPlayer::process(boost::shared_ptr<CloseFileReq> event)
{
}

void MediaPlayer::process(boost::shared_ptr<CloseFileResp> event)
{
}

void MediaPlayer::process(boost::shared_ptr<NoAudioStream> event)
{
    videoOutput->queue_event(event);
}

void MediaPlayer::process(boost::shared_ptr<NoVideoStream> event)
{
    audioOutput->queue_event(event);
}

void MediaPlayer::process(boost::shared_ptr<EndOfSystemStream> event)
{
    endOfAudioStream = false;
    endOfVideoStream = false;
}

void MediaPlayer::process(boost::shared_ptr<EndOfAudioStream> event)
{
    DEBUG();
    endOfAudioStream = true;
    if (endOfVideoStream)
    {
	close();
	skipForward();
    }
}

void MediaPlayer::process(boost::shared_ptr<EndOfVideoStream> event)
{
    DEBUG();
    endOfVideoStream = true;
    if (endOfAudioStream)
    {
	close();
	skipForward();
    }
}

void MediaPlayer::process(boost::shared_ptr<AudioSyncInfo> event)
{
    // This messeage is only received for files having an audio stream only.
    process(boost::make_shared<NotificationCurrentTime>(event->pts));
}
