#include "MediaPlayer.hpp"

#include <boost/make_shared.hpp>

MediaPlayer::MediaPlayer()
{
    // Create event_processor instances:
    demuxerEventProcessor = boost::make_shared<event_processor>();
    decoderEventProcessor = boost::make_shared<event_processor>();
    outputEventProcessor = boost::make_shared<event_processor>();

    // Create event_receiver instances:
    fileReader = boost::make_shared<FileReader>(demuxerEventProcessor);
    demuxer = boost::make_shared<Demuxer>(demuxerEventProcessor);
    videoDecoder = boost::make_shared<VideoDecoder>(decoderEventProcessor);
    audioDecoder = boost::make_shared<AudioDecoder>(decoderEventProcessor);
    videoOutput = boost::make_shared<VideoOutput>(outputEventProcessor);
    audioOutput = boost::make_shared<AudioOutput>(outputEventProcessor);

    // Start each event_processor in an own thread.
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

void MediaPlayer::operator()()
{
    sendInitEvents();

    demuxer->queue_event(boost::make_shared<OpenFileEvent>("/vol/3sat.mpg"));
    // fileReader->queue_event(boost::make_shared<OpenFileEvent>("/dev/video0"));

    // Use a Smart Pointer:
    // http://www.boost.org/doc/libs/1_39_0/libs/smart_ptr/smart_ptr.htm
    // http://www.boost.org/doc/libs/1_39_0/libs/smart_ptr/intrusive_ptr.html

    // make_shared, allocate_shared (with user-supplied allocator)
    // This should be encapsulated to allow switching between both solutions without changing user code.

    // demuxer->queue_event(new Start()); // OK: This fails to compile.

    boost::shared_ptr<StartEvent> startEvent(new StartEvent());
    demuxer->queue_event(startEvent);
    videoDecoder->queue_event(startEvent);
    audioDecoder->queue_event(startEvent);
    videoOutput->queue_event(startEvent);
    audioOutput->queue_event(startEvent);

    while(1)
    {
	sleep(10);
    }
}

void MediaPlayer::sendInitEvents()
{
     boost::shared_ptr<InitEvent> initEvent(new InitEvent());

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
