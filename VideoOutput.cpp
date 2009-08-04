#include "VideoOutput.hpp"
#include "VideoDecoder.hpp"

#include <boost/make_shared.hpp>

using namespace std;

void VideoOutput::process(boost::shared_ptr<InitEvent> event)
{
    DEBUG();
    videoDecoder = event->videoDecoder;
}

void VideoOutput::process(boost::shared_ptr<StartEvent> event)
{
    DEBUG();
}

void VideoOutput::process(boost::shared_ptr<OpenVideoOutputReq> event)
{
    DEBUG();
    
    xfVideo = boost::make_shared<XFVideo>(event->width, event->height);
    for (int i=0; i<10; i++)
    {
	createVideoImage();
    }
}

void VideoOutput::process(boost::shared_ptr<ResizeVideoOutputReq> event)
{
    DEBUG();
    xfVideo->resize(event->width, event->height);
    createVideoImage();
}

void VideoOutput::process(boost::shared_ptr<XFVideoImage> event)
{
    frameQueue.push(event);

    switch (state)
    {
    case IDLE:
	displayNextFrame();
	state = INIT;
	break;

    case INIT:
	startFrameTimer();
	break;

    default:
	break;
    }
}

void VideoOutput::process(boost::shared_ptr<DeleteXFVideoImage> event)
{
    // This class is running in the GUI thread. Here it is safe to delete
    // X11 resources. The XvImage contained in the XFVideoImage object
    // is deleted by not storing the shared_ptr.
}

void VideoOutput::process(boost::shared_ptr<ShowNextFrame> event)
{
    DEBUG();
    displayNextFrame();
}

void VideoOutput::createVideoImage()
{
    videoDecoder->queue_event(boost::make_shared<XFVideoImage>(xfVideo));
}

void VideoOutput::displayNextFrame()
{
    if (frameQueue.empty())
    {
	// No frame available to display.
	state = INIT;
	return;
    }

    boost::shared_ptr<XFVideoImage> image(frameQueue.front());
    frameQueue.pop();

    xfVideo->show(image);
    displayedPTS = image->getPTS();
    displayedTime = frameTimer.get_current_time();
    
    videoDecoder->queue_event(image);

    startFrameTimer();
}

void VideoOutput::startFrameTimer()
{
    if (frameQueue.empty())
    {
	// No frame available to calculate time.
	state = INIT;
	return;
    }

    boost::shared_ptr<XFVideoImage> image(frameQueue.front());

#if 0
    // Single shot relative timer:
    double imagePTS = image->getPTS();
    timespec_t currentTime = frameTimer.get_current_time();
    double period = imagePTS-displayedPTS;
    timespec_t dt = displayedTime-currentTime+getTimespec(period);

    std::cout << "dt=" << dt
	
	      << ", currentTime=" << currentTime 
	      << ", displayedTime=" << displayedTime
	      << ", period="<<period<< getTimespec(period)
	
	      << ", imagePTS=" << imagePTS
	      << ", displayedPTS=" << displayedPTS << std::endl;

    frameTimer.relative(dt);
    std::cout << "starting ShowNextFrame timer: sec=" << dt << std::endl;
    start_timer(boost::make_shared<ShowNextFrame>(), frameTimer);
#else
    // Periodic timer:
    if (state != RUNNING)
    {
	double imagePTS = image->getPTS();
	double period = imagePTS-displayedPTS;
	timespec_t dt = getTimespec(period);
	frameTimer.relative(dt).periodic(dt);
	start_timer(boost::make_shared<ShowNextFrame>(), frameTimer);
    }
#endif
    state = RUNNING;
}
