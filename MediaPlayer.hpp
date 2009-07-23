#ifndef MEDIA_PLAYER_HPP
#define MEDIA_PLAYER_HPP

#include "FileReader.hpp"
#include "Demuxer.hpp"
#include "VideoDecoder.hpp"
#include "AudioDecoder.hpp"
#include "VideoOutput.hpp"
#include "AudioOutput.hpp"

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>

class MediaPlayer
{
public:
    MediaPlayer();
    ~MediaPlayer();

    void operator()();

private:
    // EventReceiver
    boost::shared_ptr<FileReader> fileReader;
    boost::shared_ptr<Demuxer> demuxer;
    boost::shared_ptr<VideoDecoder> videoDecoder;
    boost::shared_ptr<AudioDecoder> audioDecoder;
    boost::shared_ptr<VideoOutput> videoOutput;
    boost::shared_ptr<AudioOutput> audioOutput;

    // Boost threads:
    boost::thread thread1;
    boost::thread thread2;
    // boost::thread demuxerThread;
    // boost::thread videoDecoderThread;
    // boost::thread audioDecoderThread;
    // boost::thread videoOutputThread;
    // boost::thread audioOutputThread;

    // EventProcessor:
    boost::shared_ptr<event_processor> eventProcessor1;
    boost::shared_ptr<event_processor> eventProcessor2;

    void sendInitEvents();
    
};

#endif
