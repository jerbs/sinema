//
// Video Output
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef VIDEO_OUTPUT_HPP
#define VIDEO_OUTPUT_HPP

#include "player/GeneralEvents.hpp"
#include "platform/event_receiver.hpp"
#include "player/XlibFacade.hpp"

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
    friend class event_processor<>;

public:
    VideoOutput(event_processor_ptr_type evt_proc)
	: base_type(evt_proc),
	  state(IDLE),
	  audioSync(false),
	  audioSnapshotPTS(0),
	  ignoreAudioSync(0),
	  lastNotifiedTime(-1),
	  displayedFramePTS(0)
    {
	audioSnapshotTime.tv_sec  = 0; 
	audioSnapshotTime.tv_nsec = 0;
    }
    ~VideoOutput()
    {
    }

private:
    MediaPlayer* mediaPlayer;
    boost::shared_ptr<Demuxer> demuxer;
#ifdef SYNCTEST
    boost::shared_ptr<SyncTest> syncTest;
#else
    boost::shared_ptr<VideoDecoder> videoDecoder;
#endif

    timer frameTimer;

    boost::shared_ptr<XFVideo> xfVideo;
    std::queue<boost::shared_ptr<XFVideoImage> > frameQueue;

    typedef enum {
	IDLE,
	INIT,
	OPEN,
	PAUSE,
	STILL,
	PLAYING
    } state_t;

    state_t state;

    bool isOpen() {return (state >= OPEN) ? true : false;}

    bool audioSync;
    double audioSnapshotPTS;
    timespec_t audioSnapshotTime;
    int ignoreAudioSync;

    boost::shared_ptr<AudioSyncInfo> audioSyncInfo;

    int lastNotifiedTime;

    double displayedFramePTS;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<OpenVideoOutputReq> event);
    void process(boost::shared_ptr<CloseVideoOutputReq> event);
    void process(boost::shared_ptr<ResizeVideoOutputReq> event);
    void process(boost::shared_ptr<XFVideoImage> event);
    void process(boost::shared_ptr<DeleteXFVideoImage> event);
    void process(boost::shared_ptr<ShowNextFrame> event);
    void process(boost::shared_ptr<AudioSyncInfo> event);
    void process(boost::shared_ptr<FlushReq> event);
    void process(boost::shared_ptr<AudioFlushedInd> event);
    void process(boost::shared_ptr<SeekRelativeReq> event);

    void process(boost::shared_ptr<CommandPlay> event);
    void process(boost::shared_ptr<CommandPause> event);

    void createVideoImage();
    void displayNextFrame();
    void startFrameTimer();
};

#endif
