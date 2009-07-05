#include "MediaPlayer.hh"

#include "Demuxer.hh"
#include "VideoDecoder.hh"
#include "AudioDecoder.hh"
#include "VideoOutput.hh"
#include "AudioOutput.hh"

MediaPlayer::MediaPlayer()
{
    // The created threads will get copies of these objects:
    Demuxer demuxer;
    VideoDecoder videoDecoder;
    AudioDecoder audioDecoder;
    VideoOutput videoOutput;
    AudioOutput audioOutput;

    demuxerThread = boost::thread(demuxer);
    videoDecoderThread = boost::thread(videoDecoder);
    audioDecoderThread = boost::thread(audioDecoder);
    videoOutputThread = boost::thread(videoOutput);
    audioOutputThread = boost::thread(audioOutput);
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
