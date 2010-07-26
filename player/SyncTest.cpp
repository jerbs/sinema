//
// Audio/Video Synchronization Test
//
// Copyright (C) Joachim Erbs, 2009, 2010
//

#include "player/SyncTest.hpp"
#include "player/AudioOutput.hpp"
#include "player/VideoOutput.hpp"
#include "player/AlsaFacade.hpp"
#include "player/XlibFacade.hpp"
#include "player/XlibHelpers.hpp"

#include <string.h>
#include <math.h>
#include <stdint.h>

SyncTest::SyncTest(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      m_pts(0)
{
}

SyncTest::~SyncTest()
{
}

void SyncTest::process(boost::shared_ptr<InitEvent> event)
{
    TRACE_DEBUG();
    audioOutput = event->audioOutput;
    videoOutput = event->videoOutput;
}

void SyncTest::process(boost::shared_ptr<StartTest> event)
{
    m_conf = *event;

    boost::shared_ptr<OpenAudioOutputReq> audiReq(new OpenAudioOutputReq());
    audiReq->sample_rate = event->sample_rate;
    audiReq->channels = event->channels;
    audiReq->frame_size = event->sample_rate * event->channels * 2;
    audioOutput->queue_event(audiReq);

    boost::shared_ptr<OpenVideoOutputReq> videoReq(new OpenVideoOutputReq(m_conf.width, m_conf.height,
									  1, 1, m_conf.imageFormat));
    videoOutput->queue_event(videoReq);
}

void SyncTest::process(boost::shared_ptr<AFAudioFrame> event)
{
    audioFrameQueue.push(event);
    generate();
}

void SyncTest::process(std::unique_ptr<XFVideoImage> event)
{
    if (event->width() != m_conf.width ||
	event->height() != m_conf.height ||
	event->xvImage()->id != m_conf.imageFormat )
    {
	// Delete frame with wrong size and or format by not queuing it
	// and request a new frame with correct size and format.
	videoOutput->queue_event(boost::make_shared<ResizeVideoOutputReq>(m_conf.width, m_conf.height,
									  1, 1, m_conf.imageFormat));
	return;
    }

    videoFrameQueue.push(std::move(event));
    generate();
}

void SyncTest::generate()
{
    while ( !audioFrameQueue.empty() &&
	    !videoFrameQueue.empty() )
    {
	boost::shared_ptr<AFAudioFrame> audioFrame(audioFrameQueue.front());
	audioFrameQueue.pop();

	std::unique_ptr<XFVideoImage> videoFrame(std::move(videoFrameQueue.front()));
	videoFrameQueue.pop();

	generateAudioFrame(audioFrame);
	generateVideoFrame(videoFrame.get());

	audioFrame->setPTS(m_pts);
	videoFrame->setPTS(m_pts);

	audioOutput->queue_event(audioFrame);
	videoOutput->queue_event(std::move(videoFrame));

	m_pts += 1;
    }
}

void SyncTest::generateAudioFrame(boost::shared_ptr<AFAudioFrame> audioFrame)
{
    int byteSize = m_conf.sample_rate * m_conf.channels * 2;

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
    double f = cDur[int(m_pts) % 8];

    for (int t=0; t<m_conf.sample_rate; t++)
    {
	
	double s = 10000*sin(f*2*M_PI*(double)t/(double)m_conf.sample_rate);
	frame[t].leftSample = s;
	frame[t].rightSample = s;
    }
}

void SyncTest::generateVideoFrame(XFVideoImage* videoFrame)
{
    //unsigned int& width = m_conf.width;
    //unsigned int& height = m_conf.height;

    int num = int(m_pts) % 8;
    //int lum = 25 + (num) * 25;

    XvImage* yuvImage = videoFrame->xvImage();

    TRACE_DEBUG(<< "yuvImage = " << std::hex << uint64_t(yuvImage));
    TRACE_DEBUG(<< "yuvImage->id = 0x" << std::hex << yuvImage->id);
    for (int i=0; i<yuvImage->num_planes; i++)
	TRACE_DEBUG(<< "yuvImage->offsets["<< i <<"] = " << yuvImage->offsets[i]);

    char* data = yuvImage->data;

    if (yuvImage->id == GUID_YUV12_PLANAR)
    {
	char* Y = data + yuvImage->offsets[0];
	//char* V = data + yuvImage->offsets[1];
	//char* U = data + yuvImage->offsets[2];

	int Yp = yuvImage->pitches[0];
	//int Vp = yuvImage->pitches[1];
	//int Up = yuvImage->pitches[2];

	videoFrame->createDemoImage();

	for (int i=0; i<=num; i++)
	{
	    Y[2*Yp+10+i*10] = 0;
	    Y[2*Yp+11+i*10] = 0;
	    Y[3*Yp+10+i*10] = 0;
	    Y[3*Yp+11+i*10] = 0;
	    Y[4*Yp+10+i*10] = 255;
	    Y[4*Yp+11+i*10] = 255;
	    Y[5*Yp+10+i*10] = 255;
	    Y[5*Yp+11+i*10] = 255;
	}
    }
    else if (yuvImage->id == GUID_YUY2_PACKED)
    {
	videoFrame->createDemoImage();

	char* P = data + yuvImage->offsets[0];
	int p = yuvImage->pitches[0];

	for (int i=0; i<=num; i++)
	{
	    P[2*p+20+i*20] = 0;
	    P[2*p+22+i*20] = 0;
	    P[3*p+20+i*20] = 0;
	    P[3*p+22+i*20] = 0;
	    P[4*p+20+i*20] = 255;
	    P[4*p+22+i*20] = 255;
	    P[5*p+20+i*20] = 255;
	    P[5*p+22+i*20] = 255;
	}
	
    }
}

// -------------------------------------------------------------------

SyncTestApp::SyncTestApp()
    : m_width(400),
      m_heigth(200),
      m_imageFormat(GUID_YUV12_PLANAR),
      // m_imageFormat(GUID_YUY2_PACKED),
      m_window(new XFWindow(m_width, m_heigth))
{
    // Create event_processor instances:
    testEventProcessor = boost::make_shared<event_processor<> >();
    outputEventProcessor = boost::make_shared<event_processor<> >();

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
    startTest->width  = m_width;
    startTest->height = m_heigth;
    startTest->imageFormat = m_imageFormat;

    test->queue_event(startTest);

    while(1)
    {
	sleep(10);
    }
}

void SyncTestApp::sendInitEvents()
{
     boost::shared_ptr<InitEvent> initEvent(new InitEvent());

     initEvent->mediaPlayer = test.get();
     initEvent->demuxer = test;
     initEvent->videoDecoder = test;
     initEvent->audioDecoder = test;
     initEvent->videoOutput = videoOutput;
     initEvent->audioOutput = audioOutput;

     test->queue_event(initEvent);
     videoOutput->queue_event(initEvent);
     audioOutput->queue_event(initEvent);

     videoOutput->queue_event(boost::make_shared<WindowRealizeEvent>(m_window->display(), m_window->window()));
     videoOutput->queue_event(boost::make_shared<WindowConfigureEvent>(0,0, m_width, m_heigth));
}

int main()
{
    SyncTestApp syncTestApp;
    syncTestApp();

    return 0;
}
