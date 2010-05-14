//
// Media Recorder
//
// Copyright (C) Joachim Erbs
//

#ifndef MEDIA_RECORDER_HPP
#define MEDIA_RECORDER_HPP

#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"
#include "recorder/GeneralEvents.hpp"

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

class MediaRecorderThreadNotification
{
public:
    typedef void (*fct_t)();

    MediaRecorderThreadNotification();
    static void setCallback(fct_t fct);

private:
    static fct_t m_fct;
};

class MediaRecorder : public event_receiver<MediaRecorder,
                                            concurrent_queue<receive_fct_t, MediaRecorderThreadNotification> >
{
    // The friend declaration allows to define the process methods private:
    friend class event_processor<concurrent_queue<receive_fct_t, MediaRecorderThreadNotification> >;
    friend class MediaRecorderThreadNotification;

public:
    MediaRecorder();
    ~MediaRecorder();

    void init();

    void processEventQueue();

protected:
    // EventReceiver
    boost::shared_ptr<Recorder> recorder;
    boost::shared_ptr<RecorderAdapter> recorderAdapter;

private:
    // Boost threads:
    boost::thread recorderThread;

    // EventProcessor:
    boost::shared_ptr<event_processor<> > recorderEventProcessor;

    void sendInitEvents();
};

#endif
