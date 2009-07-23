#include "MediaPlayer.hpp"

#include <boost/make_shared.hpp>

MediaPlayer::MediaPlayer()
{
    // Create event_processor instances:
    eventProcessor1 = boost::make_shared<event_processor>();
    eventProcessor2 = boost::make_shared<event_processor>();

    // Create event_receiver instances:
    fileReader = boost::make_shared<FileReader>(eventProcessor1);
    demuxer = boost::make_shared<Demuxer>(eventProcessor1);
    videoDecoder = boost::make_shared<VideoDecoder>(eventProcessor1);
    audioDecoder = boost::make_shared<AudioDecoder>(eventProcessor1);
    videoOutput = boost::make_shared<VideoOutput>(eventProcessor2);
    audioOutput = boost::make_shared<AudioOutput>(eventProcessor2);

    // Start each event_processor in an own thread.
    // Demuxer has a custom main loop:
    thread1 = boost::thread( eventProcessor1->get_callable(demuxer) );
    thread2 = boost::thread( eventProcessor2->get_callable() );
}

MediaPlayer::~MediaPlayer()
{
    boost::shared_ptr<QuitEvent> quitEvent(new QuitEvent());
    eventProcessor1->queue_event(quitEvent);
    eventProcessor2->queue_event(quitEvent);

    thread1.join();
    thread2.join();

    // demuxerThread.join();
    // videoDecoderThread.join();
    // audioDecoderThread.join();
    // videoOutputThread.join();
    // audioOutputThread.join();
}

void MediaPlayer::operator()()
{
    sendInitEvents();

    fileReader->queue_event(boost::make_shared<OpenFileEvent>("/vol/3sat.mpg"));
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

    sleep(10);
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
