//
// Video Output
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef VIDEO_OUTPUT_HPP
#define VIDEO_OUTPUT_HPP

#include "player/GeneralEvents.hpp"
#include "platform/event_receiver.hpp"

#include <sys/ipc.h>  // to allocate shared memory
#include <sys/shm.h>  // to allocate shared memory

#include <queue>

#include <boost/shared_ptr.hpp>

struct ShowNextFrame {};

class XFVideo;
class XFVideoImage;

class VideoOutput : public event_receiver<VideoOutput,
					  concurrent_queue<receive_fct_t, MediaPlayerThreadNotification> >
{
    friend class event_processor<concurrent_queue<receive_fct_t, MediaPlayerThreadNotification> >;

public:
    VideoOutput(event_processor_ptr_type evt_proc)
	: base_type(evt_proc),
	  eos(false),
	  state(IDLE),
	  audioSync(false),
	  audioSnapshotPTS(0),
	  ignoreAudioSync(0),
	  videoStreamOnly(false),
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

    bool eos;

    typedef enum {
	IDLE,
	INIT,
	OPEN,
	PAUSE,
	FLUSHED,
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

    bool videoStreamOnly;

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
    void process(boost::shared_ptr<EndOfVideoStream> event);		 
    void process(boost::shared_ptr<NoAudioStream> event);		 
    void process(boost::shared_ptr<WindowRealizeEvent> event);
    void process(boost::shared_ptr<WindowConfigureEvent> event);
    void process(boost::shared_ptr<WindowExposeEvent> event);
    void process(boost::shared_ptr<ClipVideoEvent> event);

    void process(boost::shared_ptr<CommandPlay> event);
    void process(boost::shared_ptr<CommandPause> event);

    void createVideoImage();
    void displayNextFrame();
    void startFrameTimer();

    void showBlackFrame();

    void sendNotificationVideoSize(boost::shared_ptr<NotificationVideoSize> event);
};

#endif
