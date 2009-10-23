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
#include "player/PlayList.hpp"

#include "platform/event_receiver.hpp"

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

class MediaPlayerThreadNotification
{
public:
    typedef void (*fct_t)();

    MediaPlayerThreadNotification();
    static void setCallback(fct_t fct);

private:
    static fct_t m_fct;
};

class MediaPlayer : public event_receiver<MediaPlayer,
					  concurrent_queue<receive_fct_t, MediaPlayerThreadNotification> >
{
    // The friend declaration allows to define the process methods private:
    friend class event_processor<concurrent_queue<receive_fct_t, MediaPlayerThreadNotification> >;
    friend class MediaPlayerThreadNotification;

public:
    MediaPlayer(boost::shared_ptr<PlayList> playList);
    ~MediaPlayer();

    void init();

    void open();
    void close();

    void play();
    void pause();

    void skipBack();
    void skipForward();

    void seekAbsolute(double second);
    void seekRelative(double secondsDelta);

    void setPlaybackVolume(double volume);
    void setPlaybackSwitch(bool enabled);

    // protected:
    void processEventQueue();

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
    boost::shared_ptr<event_processor<> > demuxerEventProcessor;
    boost::shared_ptr<event_processor<> > decoderEventProcessor;
    boost::shared_ptr<event_processor<> > outputEventProcessor;

    // PlayList:
    boost::shared_ptr<PlayList> playList;

    void sendInitEvents();

    void process(boost::shared_ptr<EndOfSystemStream> event);
    void process(boost::shared_ptr<EndOfAudioStream> event);
    void process(boost::shared_ptr<EndOfVideoStream> event);

    bool endOfAudioStream;
    bool endOfVideoStream;

    virtual void process(boost::shared_ptr<NotificationFileInfo> event) = 0;
    virtual void process(boost::shared_ptr<NotificationCurrentTime> event) = 0;
    virtual void process(boost::shared_ptr<NotificationCurrentVolume> event) = 0;
};

#endif
