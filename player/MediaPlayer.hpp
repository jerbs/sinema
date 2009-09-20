//
// Media Player
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef MEDIA_PLAYER_HPP
#define MEDIA_PLAYER_HPP

#include "player/FileReader.hpp"
#include "player/Demuxer.hpp"
#include "player/VideoDecoder.hpp"
#include "player/AudioDecoder.hpp"
#include "player/VideoOutput.hpp"
#include "player/AudioOutput.hpp"

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

class MediaPlayer
{
public:
    MediaPlayer();
    ~MediaPlayer();

    void operator()(std::string file);

    void play();
    void pause();
    void stop();

private:
    // EventReceiver
    boost::shared_ptr<FileReader> fileReader;
    boost::shared_ptr<Demuxer> demuxer;
    boost::shared_ptr<VideoDecoder> videoDecoder;
    boost::shared_ptr<AudioDecoder> audioDecoder;
    boost::shared_ptr<VideoOutput> videoOutput;
    boost::shared_ptr<AudioOutput> audioOutput;

    // Boost threads:
    boost::thread demuxerThread;
    boost::thread decoderThread;
    boost::thread outputThread;
    // boost::thread videoDecoderThread;
    // boost::thread audioDecoderThread;
    // boost::thread videoOutputThread;
    // boost::thread audioOutputThread;

    // EventProcessor:
    boost::shared_ptr<event_processor> demuxerEventProcessor;
    boost::shared_ptr<event_processor> decoderEventProcessor;
    boost::shared_ptr<event_processor> outputEventProcessor;

    void sendInitEvents();
    
};

#endif
