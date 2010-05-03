//
// Media Player
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef MEDIA_PLAYER_HPP
#define MEDIA_PLAYER_HPP

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

#include "player/Demuxer.hpp"
#include "player/VideoDecoder.hpp"
#include "player/AudioDecoder.hpp"
#include "player/VideoOutput.hpp"
#include "player/AudioOutput.hpp"

#include "platform/event_receiver.hpp"

class PlayList;

class MediaPlayer : public event_receiver<MediaPlayer,
					  concurrent_queue<receive_fct_t, MediaPlayerThreadNotification> >
{
    // The friend declaration allows to define the process methods private:
    friend class event_processor<concurrent_queue<receive_fct_t, MediaPlayerThreadNotification> >;
    friend class MediaPlayerThreadNotification;

public:
    MediaPlayer(PlayList& playList);
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

    void clip(boost::shared_ptr<ClipVideoDstEvent> event);
    void clip(boost::shared_ptr<ClipVideoSrcEvent> event);

    void processEventQueue();

protected:
    // EventReceiver
    boost::shared_ptr<Demuxer> demuxer;
    boost::shared_ptr<VideoDecoder> videoDecoder;
    boost::shared_ptr<AudioDecoder> audioDecoder;
    boost::shared_ptr<VideoOutput> videoOutput;
    boost::shared_ptr<AudioOutput> audioOutput;

private:
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
    PlayList& m_PlayList;

    void sendInitEvents();

    void process(boost::shared_ptr<OpenFileResp> event);
    void process(boost::shared_ptr<OpenFileFail> event);
    virtual void process(boost::shared_ptr<CloseFileResp> event);

    void process(boost::shared_ptr<NoAudioStream> event);
    void process(boost::shared_ptr<NoVideoStream> event);

    void process(boost::shared_ptr<EndOfSystemStream> event);
    void process(boost::shared_ptr<EndOfAudioStream> event);
    void process(boost::shared_ptr<EndOfVideoStream> event);

    void process(boost::shared_ptr<AudioSyncInfo> event);

    bool skipForwardInt();

    bool endOfAudioStream;
    bool endOfVideoStream;

    virtual void process(boost::shared_ptr<NotificationFileInfo> event) = 0;
    virtual void process(boost::shared_ptr<NotificationCurrentTime> event) = 0;
    virtual void process(boost::shared_ptr<NotificationCurrentVolume> event) = 0;
    virtual void process(boost::shared_ptr<NotificationVideoSize> event) = 0;
    virtual void process(boost::shared_ptr<NotificationClipping> event) = 0;

    virtual void process(boost::shared_ptr<OpenAudioStreamFailed> event) {};
    virtual void process(boost::shared_ptr<OpenVideoStreamFailed> event) {};

    virtual void process(boost::shared_ptr<HideCursorEvent> event) = 0;
};

#endif
