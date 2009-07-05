#ifndef MEDIA_PLAYER_HH
#define MEDIA_PLAYER_HH

#include <boost/thread/thread.hpp>

class MediaPlayer
{
public:
    MediaPlayer();
    ~MediaPlayer();

    void operator()();

private:
    boost::thread demuxerThread;
    boost::thread videoDecoderThread;
    boost::thread audioDecoderThread;
    boost::thread videoOutputThread;
    boost::thread audioOutputThread;
};

#endif
