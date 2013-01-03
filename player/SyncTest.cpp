//
// Audio/Video Synchronization Test
//
// Copyright (C) Joachim Erbs, 2009-2010
//
//    This file is part of Sinema.
//
//    Sinema is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Sinema is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Sinema.  If not, see <http://www.gnu.org/licenses/>.
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
    TRACE_DEBUG(<< "tid = " << gettid());

    audioOutput = event->audioOutput;
    videoOutput = event->videoOutput;
}

void SyncTest::process(boost::shared_ptr<StartTest> event)
{
    m_conf = *event;

    boost::shared_ptr<OpenAudioOutputReq>
	audiReq(new OpenAudioOutputReq(event->sample_rate,
				       event->channels,
				       SAMPLE_FMT_S16,
				       event->sample_rate * event->channels * 2));
    audioOutput->queue_event(audiReq);

    boost::shared_ptr<OpenVideoOutputReq> videoReq(new OpenVideoOutputReq(m_conf.width, m_conf.height,
									  1, 1, m_conf.imageFormat));
    videoOutput->queue_event(videoReq);
}

void SyncTest::process(boost::shared_ptr<AudioFrame> event)
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
	// Delete frame with wrong size and or format in X11 thread:
	videoOutput->queue_event(std::unique_ptr<DeleteXFVideoImage>(new DeleteXFVideoImage(std::move(event))));

	// Request a new frame with correct size and format.
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
	boost::shared_ptr<AudioFrame> audioFrame(audioFrameQueue.front());
	audioFrameQueue.pop();

	std::unique_ptr<XFVideoImage> videoFrame(std::move(videoFrameQueue.front()));
	videoFrameQueue.pop();

	switch (m_conf.mode)
	{
	case StartTest::Tonleiter:
	    generateAudioFrameTonleiter(audioFrame);
	    generateVideoFrameTonleiter(videoFrame.get());
	    break;
	case StartTest::Tick:
	    generateAudioFrameTick(audioFrame);
	    generateVideoFrameTick(videoFrame.get());
	    break;
	}

	audioFrame->setPTS(m_pts);
	videoFrame->setPTS(m_pts);

	audioOutput->queue_event(audioFrame);
	videoOutput->queue_event(std::move(videoFrame));

	m_pts += 1.0/float(m_conf.frames_per_second);
    }
}

void SyncTest::generateAudioFrameTick(boost::shared_ptr<AudioFrame> audioFrame)
{
    int samples_per_frame = m_conf.sample_rate / m_conf.frames_per_second;
    int byteSize = samples_per_frame * m_conf.channels * 2;

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

    frame_t* frame = (frame_t*)audioFrame->data();

    int phase = int(float(m_pts) * float(m_conf.frames_per_second)) % m_conf.frames_per_second;

    if (phase == m_conf.frames_per_second / 2)
    {
	double f = 440;
	for (int t=0; t<samples_per_frame; t++)
	{
	    double s = 10000*sin(f*2*M_PI*(double)t/(double)m_conf.sample_rate);
	    frame[t].leftSample = s;
	    frame[t].rightSample = s;
	}
    }
    else
    {
	for (int t=0; t<samples_per_frame; t++)
	{
	    frame[t].leftSample = 0;
	    frame[t].rightSample = 0;
	}
    }
}

void SyncTest::generateVideoFrameTick(XFVideoImage* videoFrame)
{
    int phase = int(float(m_pts) * float(m_conf.frames_per_second)) % m_conf.frames_per_second;

    int color1 = 0;
    int color2 = 255;
    if (phase == m_conf.frames_per_second / 2)
    {
	color1 = 255;
	color2 = 0;
    }

    XvImage* yuvImage = videoFrame->xvImage();

    int w = videoFrame->width();
    int h = videoFrame->height();

        if (yuvImage->id == GUID_YUV12_PLANAR)
    {
        char* Y = videoFrame->data() + yuvImage->offsets[0];
        char* V = videoFrame->data() + yuvImage->offsets[1];
        char* U = videoFrame->data() + yuvImage->offsets[2];

	for (int y=0; y<h; y++)
	{
	    for (int x=0; x<w; x++)
            {
                Y[x + y*yuvImage->pitches[0]] = color1;
            }
	}

        for (int x=0; x<w/2; x++)
            for (int y=0; y<h/2; y++)
            {
                U[x + y*yuvImage->pitches[2]] = 128;
                V[x + y*yuvImage->pitches[1]] = 128;
            }
    }
    else if (yuvImage->id == GUID_YUY2_PACKED)
    {
        char* Packed = videoFrame->data() + yuvImage->offsets[0];

	for (int y=0; y<h; y++)
	{
	    for (int x=0; x<w; x++)
            {
                Packed[2*x + y*yuvImage->pitches[0]] = 128;  // Y
            }
	}

        for (int x=0; x<w/2; x++)
            for (int y=0; y<h; y++)
            {
                Packed[4*x+1 + y*yuvImage->pitches[0]] = 128; // U
                Packed[4*x+3 + y*yuvImage->pitches[0]] = 128;   // V
            }
    }
    else
    {
        TRACE_THROW(std::string, << "unsupported format 0x" << std::hex << yuvImage->id);
    }
}

void SyncTest::generateAudioFrameTonleiter(boost::shared_ptr<AudioFrame> audioFrame)
{
    int samples_per_frame = m_conf.sample_rate / m_conf.frames_per_second;
    int byteSize = samples_per_frame * m_conf.channels * 2;

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

    for (int t=0; t<samples_per_frame; t++)
    {
	
	double s = 10000*sin(f*2*M_PI*(double)t/(double)m_conf.sample_rate);
	frame[t].leftSample = s;
	frame[t].rightSample = s;
    }
}

void SyncTest::generateVideoFrameTonleiter(XFVideoImage* videoFrame)
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
    audioOutputEventProcessor = boost::make_shared<event_processor<> >();
    videoOutputEventProcessor = boost::make_shared<event_processor<concurrent_queue<receive_fct_t, with_callback_function> > >();

    // Create event_receiver instances:
    test = boost::make_shared<SyncTest>(testEventProcessor);
    videoOutput = boost::make_shared<VideoOutput>(videoOutputEventProcessor);
    audioOutput = boost::make_shared<AudioOutput>(audioOutputEventProcessor);

    // Start each event_processor in an own thread.
    // Demuxer has a custom main loop:
    testThread = boost::thread( testEventProcessor->get_callable() );
    audioOutputThread  = boost::thread( audioOutputEventProcessor->get_callable() );
    videoOutputThread  = boost::thread( videoOutputEventProcessor->get_callable() );
}

SyncTestApp::~SyncTestApp()
{
}

void SyncTestApp::operator()()
{
    sendInitEvents();

    boost::shared_ptr<StartTest> startTest(new StartTest());
    startTest->frames_per_second = 25;
    //startTest->frames_per_second = 2;
    startTest->sample_rate = 48000;
    startTest->channels = 2;
    startTest->width  = m_width;
    startTest->height = m_heigth;
    startTest->imageFormat = m_imageFormat;
    startTest->mode = StartTest::Tick;
    //startTest->mode = StartTest::Tonleiter;

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
     initEvent->deinterlacer = test;
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
