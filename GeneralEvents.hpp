#ifndef GENERAL_EVENTS_HPP
#define GENERAL_EVENTS_HPP

#include <boost/shared_ptr.hpp>
#include <iostream>

#define DEBUG(x) std::cout << __PRETTY_FUNCTION__  x << std::endl

class FileReader;
class Demuxer;
class VideoDecoder;
class AudioDecoder;
class VideoOutput;
class AudioOutput;

struct InitEvent
{
    boost::shared_ptr<FileReader> fileReader;
    boost::shared_ptr<Demuxer> demuxer;
    boost::shared_ptr<VideoDecoder> videoDecoder;
    boost::shared_ptr<AudioDecoder> audioDecoder;
    boost::shared_ptr<VideoOutput> videoOutput;
    boost::shared_ptr<AudioOutput> audioOutput;
};

struct QuitEvent
{
    QuitEvent(){DEBUG();}
    ~QuitEvent(){DEBUG();}
};

struct StartEvent
{
    StartEvent(){DEBUG();}
    ~StartEvent(){DEBUG();}
};

struct StopEvent
{
    StopEvent(){DEBUG();}
    ~StopEvent(){DEBUG();}
};

#endif
