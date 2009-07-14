#include "MediaPlayer.hh"

#include "Demuxer.hh"
#include "VideoDecoder.hh"
#include "AudioDecoder.hh"
#include "VideoOutput.hh"
#include "AudioOutput.hh"

#include <boost/make_shared.hpp>


MediaPlayer::MediaPlayer()
{
    Demuxer* demuxer = new Demuxer();
    // The created threads will get copies of these objects:
    VideoDecoder videoDecoder;
    AudioDecoder audioDecoder;
    VideoOutput videoOutput;
    AudioOutput audioOutput;

    demuxerThread = boost::thread( boost::bind(&Demuxer::operator(), demuxer ) );
    videoDecoderThread = boost::thread(videoDecoder);
    audioDecoderThread = boost::thread(audioDecoder);
    videoOutputThread = boost::thread(videoOutput);
    audioOutputThread = boost::thread(audioOutput);

    // Use a Smart Pointer:
    // http://www.boost.org/doc/libs/1_39_0/libs/smart_ptr/smart_ptr.htm
    // http://www.boost.org/doc/libs/1_39_0/libs/smart_ptr/intrusive_ptr.html

    // make_shared, allocate_shared (with user-supplied allocator)
    // This should be encapsulated to allow switching between both solutions without changing user code.

    // demuxer->queue_event(new Start()); // OK: This fails to compile.
    demuxer->queue_event(boost::make_shared<Start>());
    demuxer->queue_event(boost::make_shared<Stop>());
    demuxer->queue_event(boost::make_shared<Quit>());
}

MediaPlayer::~MediaPlayer()
{
    demuxerThread.join();
    videoDecoderThread.join();
    audioDecoderThread.join();
    videoOutputThread.join();
    audioOutputThread.join();
}

void MediaPlayer::operator()()
{

}
