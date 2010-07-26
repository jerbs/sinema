//
// Audio/Video Synchronization Test
//
// Copyright (C) Joachim Erbs, 2009, 2010
//

#ifndef SYNC_TEST_HPP
#define SYNC_TEST_HPP

#include "player/GeneralEvents.hpp"
#include "platform/event_receiver.hpp"

#include <boost/shared_ptr.hpp>

class AFAudioFrame;
class XFVideoImage;
class XFWindow;

struct StartTest
{
    int sample_rate;
    int channels;

    unsigned int width;
    unsigned int height;
    int imageFormat;
};

class SyncTest : public event_receiver<SyncTest>
{
    friend class event_processor<>;

    std::queue<boost::shared_ptr<AFAudioFrame> > audioFrameQueue;
    std::queue<  std::unique_ptr<XFVideoImage> > videoFrameQueue;

    StartTest m_conf;
    double m_pts;

public:
    SyncTest(event_processor_ptr_type evt_proc);
    ~SyncTest();

private:
    boost::shared_ptr<AudioOutput> audioOutput;
    boost::shared_ptr<VideoOutput> videoOutput;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<StartTest> event);
    void process(boost::shared_ptr<AFAudioFrame> event);
    void process(  std::unique_ptr<XFVideoImage> event);

    // Stubs for MediaPlayer, Demuxer, AudioDecoder and VideoDecoder:
    void process(boost::shared_ptr<OpenAudioOutputResp>) {}
    void process(boost::shared_ptr<CloseAudioOutputResp>) {}
    void process(boost::shared_ptr<OpenVideoOutputResp>) {}
    void process(boost::shared_ptr<CloseVideoOutputResp>) {}
    void process(boost::shared_ptr<AudioSyncInfo>) {}
    void process(boost::shared_ptr<EndOfAudioStream>) {}
    void process(boost::shared_ptr<EndOfVideoStream>) {}
    void process(boost::shared_ptr<NotificationVideoSize>) {}
    void process(boost::shared_ptr<NotificationClipping>) {}
    void process(boost::shared_ptr<NotificationCurrentTime>) {}
    void process(boost::shared_ptr<NotificationCurrentVolume>) {}
    void process(boost::shared_ptr<SeekRelativeReq>) {}

    void generate();
    void generateAudioFrame(boost::shared_ptr<AFAudioFrame> audioFrame);
    void generateVideoFrame(XFVideoImage* videoFrame);
};

class SyncTestApp
{
public:
    SyncTestApp();
    ~SyncTestApp();

    void operator()();

private:
    int m_width;
    int m_heigth;
    int m_imageFormat;
    boost::shared_ptr<XFWindow> m_window;

    // EventReceiver
    boost::shared_ptr<SyncTest> test;
    boost::shared_ptr<VideoOutput> videoOutput;
    boost::shared_ptr<AudioOutput> audioOutput;

    // Boost threads:
    boost::thread testThread;
    boost::thread outputThread;

    // EventProcessor:
    boost::shared_ptr<event_processor<> > testEventProcessor;
    boost::shared_ptr<event_processor<> > outputEventProcessor;

    void sendInitEvents();
};

#endif
