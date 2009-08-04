#ifndef VIDEO_OUTPUT_HPP
#define VIDEO_OUTPUT_HPP

#include "GeneralEvents.hpp"
#include "event_receiver.hpp"
#include "XlibFacade.hpp"

#include <sys/ipc.h>  // to allocate shared memory
#include <sys/shm.h>  // to allocate shared memory

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/XShm.h>  // has to be included before Xvlib.h
#include <X11/extensions/Xvlib.h>

#include <queue>

#include <boost/shared_ptr.hpp>

struct ShowNextFrame {};

class VideoOutput : public event_receiver<VideoOutput>
{
    friend class event_processor;

public:
    VideoOutput(event_processor_ptr_type evt_proc)
	: base_type(evt_proc),
	  state(IDLE),
	  displayedPTS(0)
    {
#if 0
	// Periodic timer, every 0.1 seconds:
	timespec_t period;
	period.tv_sec  = 0;       
	period.tv_nsec = 100*1000*1000;  
	frameTimer.relative(period).periodic(period);
#endif
    }
    ~VideoOutput()
    {
    }

private:
    boost::shared_ptr<VideoDecoder> videoDecoder;

    timer frameTimer;

    boost::shared_ptr<XFVideo> xfVideo;
    std::queue<boost::shared_ptr<XFVideoImage> > frameQueue;

    typedef enum {
	IDLE,
	INIT,
	RUNNING
    } state_t;

    state_t state;
    double displayedPTS;
    timespec_t displayedTime;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<StartEvent> event);
    void process(boost::shared_ptr<OpenVideoOutputReq> event);
    void process(boost::shared_ptr<ResizeVideoOutputReq> event);
    void process(boost::shared_ptr<XFVideoImage> event);
    void process(boost::shared_ptr<DeleteXFVideoImage> event);
    void process(boost::shared_ptr<ShowNextFrame> event);

    void createVideoImage();
    void displayNextFrame();
    void startFrameTimer();
};

#endif
