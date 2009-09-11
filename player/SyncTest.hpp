#ifndef SYNC_TEST_HPP
#define SYNC_TEST_HPP

#ifdef SYNCTEST
#include "player/GeneralEvents.hpp"
#include "platform/event_receiver.hpp"
#include "player/XlibFacade.hpp"
#include "player/AlsaFacade.hpp"

#include <boost/shared_ptr.hpp>

struct StartTest
{
    int sample_rate;
    int channels;

    int width;
    int height;
};

class SyncTest : public event_receiver<SyncTest>
{
    friend class event_processor;

    std::queue<boost::shared_ptr<AFAudioFrame> > audioFrameQueue;
    std::queue<boost::shared_ptr<XFVideoImage> > videoFrameQueue;

    StartTest conf;
    double pts;

public:
    SyncTest(event_processor_ptr_type evt_proc)
	: base_type(evt_proc),
	  pts(0)
    {
    }

    ~SyncTest()
    {
    }

private:
    boost::shared_ptr<AudioOutput> audioOutput;
    boost::shared_ptr<VideoOutput> videoOutput;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<StartTest> event);
    void process(boost::shared_ptr<AFAudioFrame> event);
    void process(boost::shared_ptr<XFVideoImage> event);

    void generate();
    void generateAudioFrame(boost::shared_ptr<AFAudioFrame> audioFrame);
    void generateVideoFrame(boost::shared_ptr<XFVideoImage> videoFrame);
};

class SyncTestApp
{
public:
    SyncTestApp();
    ~SyncTestApp();

    void operator()();

private:
    // EventReceiver
    boost::shared_ptr<SyncTest> test;
    boost::shared_ptr<VideoOutput> videoOutput;
    boost::shared_ptr<AudioOutput> audioOutput;

    // Boost threads:
    boost::thread testThread;
    boost::thread outputThread;

    // EventProcessor:
    boost::shared_ptr<event_processor> testEventProcessor;
    boost::shared_ptr<event_processor> outputEventProcessor;

    void sendInitEvents();
};

#endif

#endif
