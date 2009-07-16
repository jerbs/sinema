#include "MediaPlayer.hpp"

#include <boost/make_shared.hpp>


MediaPlayer::MediaPlayer()
{
    // Create event_processor instances:
    eventProcessor1 = boost::make_shared<event_processor>();
    eventProcessor2 = boost::make_shared<event_processor>();

    // Start each event_processor in an own thread:
    thread1 = boost::thread( boost::bind(&event_processor::operator(), eventProcessor1 ) );
    thread2 = boost::thread( boost::bind(&event_processor::operator(), eventProcessor2 ) );

    // Create event_Receiver instances:
    demuxer = boost::make_shared<Demuxer>(eventProcessor1);
    videoDecoder = boost::make_shared<VideoDecoder>(eventProcessor1);
    audioDecoder = boost::make_shared<AudioDecoder>(eventProcessor1);
    videoOutput = boost::make_shared<VideoOutput>(eventProcessor2);
    audioOutput = boost::make_shared<AudioOutput>(eventProcessor2);
}

MediaPlayer::~MediaPlayer()
{
    boost::shared_ptr<Quit> quitEvent(new Quit());
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
    // Use a Smart Pointer:
    // http://www.boost.org/doc/libs/1_39_0/libs/smart_ptr/smart_ptr.htm
    // http://www.boost.org/doc/libs/1_39_0/libs/smart_ptr/intrusive_ptr.html

    // make_shared, allocate_shared (with user-supplied allocator)
    // This should be encapsulated to allow switching between both solutions without changing user code.

    // demuxer->queue_event(new Start()); // OK: This fails to compile.

    boost::shared_ptr<Start> startEvent(new Start());
    demuxer->queue_event(startEvent);
    videoDecoder->queue_event(startEvent);
    audioDecoder->queue_event(startEvent);
    videoOutput->queue_event(startEvent);
    audioOutput->queue_event(startEvent);

    demuxer->queue_event(boost::make_shared<Start>());
    demuxer->queue_event(boost::make_shared<Stop>());
}
