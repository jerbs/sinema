#ifdef SYNCTEST

#include "SyncTest.hpp"
#include "AudioOutput.hpp"
#include "VideoOutput.hpp"

#include <string.h>
#include <math.h>
#include <stdint.h>

void SyncTest::process(boost::shared_ptr<InitEvent> event)
{
    DEBUG();
    audioOutput = event->audioOutput;
    videoOutput = event->videoOutput;
}

void SyncTest::process(boost::shared_ptr<StartTest> event)
{
    conf = *event;

    boost::shared_ptr<OpenAudioOutputReq> audiReq(new OpenAudioOutputReq());
    audiReq->sample_rate = event->sample_rate;
    audiReq->channels = event->channels;
    audiReq->frame_size = event->sample_rate * event->channels * 2;
    audioOutput->queue_event(audiReq);

    boost::shared_ptr<OpenVideoOutputReq> videoReq(new OpenVideoOutputReq());
    videoReq->width = event->width;
    videoReq->height = event->height;
    videoOutput->queue_event(videoReq);
}

void SyncTest::process(boost::shared_ptr<AFAudioFrame> event)
{
    audioFrameQueue.push(event);
    generate();
}

void SyncTest::process(boost::shared_ptr<XFVideoImage> event)
{
    videoFrameQueue.push(event);
    generate();
}

void SyncTest::generate()
{
    while ( !audioFrameQueue.empty() &&
	    !videoFrameQueue.empty() )
    {
	boost::shared_ptr<AFAudioFrame> audioFrame(audioFrameQueue.front());
	audioFrameQueue.pop();

	boost::shared_ptr<XFVideoImage> videoFrame(videoFrameQueue.front());
	videoFrameQueue.pop();

	generateAudioFrame(audioFrame);
	generateVideoFrame(videoFrame);

	audioFrame->setPTS(pts);
	videoFrame->setPTS(pts);

	audioOutput->queue_event(audioFrame);
	videoOutput->queue_event(videoFrame);

	pts += 1;
    }
}

void SyncTest::generateAudioFrame(boost::shared_ptr<AFAudioFrame> audioFrame)
{
    int byteSize = conf.sample_rate * conf.channels * 2;

    audioFrame->reset();
    audioFrame->setFrameByteSize(byteSize);

    if (audioFrame->numAllocatedBytes() < byteSize)
    {
	std::cout << "Error: Audio frames are too small." << std::endl;
	exit(1);
    }

    struct frame_t
    {
	int16_t leftSample;
	int16_t rightSample;
    };

    int cDur[] = {
	264,  //  Hz 	c1
	297,  //  Hz 	d1
	330,  //  Hz 	e1
	352,  //  Hz 	f1
	396,  //  Hz 	g1
	440,  //  Hz 	a1 (Kammerton)
	495,  //  Hz 	h1 (engl.: b1)
	528}; //  Hz 	c2

    assert(8 == sizeof(cDur)/sizeof(int));

    frame_t* frame = (frame_t*)audioFrame->data();
    double f = cDur[int(pts) % 8];

    for (int t=0; t<conf.sample_rate; t++)
    {
	
	double s = 10000*sin(f*2*M_PI*(double)t/(double)conf.sample_rate);
	frame[t].leftSample = s;
	frame[t].rightSample = s;
    }
}

void SyncTest::generateVideoFrame(boost::shared_ptr<XFVideoImage> videoFrame)
{
    int& width = conf.width;
    int& height =conf.height;

    int num = int(pts) % 8;
    int lum = 25 + (num) * 25;

    char* data = videoFrame->data();

    char* Y = data;
    char* V = data + width * height;
    char* U = V + width/2 * height/2;

    memset(Y, lum, width * height);
    memset(V, 100, width/2 * height/2);
    memset(U, 160, width/2 * height/2);

    for (int i=0; i<=num; i++)
    {
	Y[2*width+10+i*10] = 0;
	Y[2*width+11+i*10] = 0;
	Y[3*width+10+i*10] = 0;
	Y[3*width+11+i*10] = 0;
	Y[4*width+10+i*10] = 255;
	Y[4*width+11+i*10] = 255;
	Y[5*width+10+i*10] = 255;
	Y[5*width+11+i*10] = 255;
    }

}

// -------------------------------------------------------------------

SyncTestApp::SyncTestApp()
{
    // Create event_processor instances:
    testEventProcessor = boost::make_shared<event_processor>();
    outputEventProcessor = boost::make_shared<event_processor>();

    // Create event_receiver instances:
    test = boost::make_shared<SyncTest>(testEventProcessor);
    videoOutput = boost::make_shared<VideoOutput>(outputEventProcessor);
    audioOutput = boost::make_shared<AudioOutput>(outputEventProcessor);

    // Start each event_processor in an own thread.
    // Demuxer has a custom main loop:
    testThread = boost::thread( testEventProcessor->get_callable() );
    outputThread  = boost::thread( outputEventProcessor->get_callable() );
}

SyncTestApp::~SyncTestApp()
{
}

void SyncTestApp::operator()()
{
    sendInitEvents();

    boost::shared_ptr<StartTest> startTest(new StartTest());
    startTest->sample_rate = 48000;
    startTest->channels = 2;
    startTest->width  = 400;
    startTest->height = 200;

    test->queue_event(startTest);

    while(1)
    {
	sleep(10);
    }
}

void SyncTestApp::sendInitEvents()
{
     boost::shared_ptr<InitEvent> initEvent(new InitEvent());

     initEvent->syncTest = test;
     initEvent->videoOutput = videoOutput;
     initEvent->audioOutput = audioOutput;

     test->queue_event(initEvent);
     videoOutput->queue_event(initEvent);
     audioOutput->queue_event(initEvent);
}

#endif
