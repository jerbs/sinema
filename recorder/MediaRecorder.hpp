//
// Media Recorder
//
// Copyright (C) Joachim Erbs, 2010
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

#ifndef MEDIA_RECORDER_HPP
#define MEDIA_RECORDER_HPP

#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"
#include "recorder/GeneralEvents.hpp"
#include "player/GeneralEvents.hpp"

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
    boost::thread recorderAdapterThread;

    // EventProcessor:
    boost::shared_ptr<event_processor< concurrent_queue<receive_fct_t, RecorderThreadNotification> > > recorderEventProcessor;
    boost::shared_ptr<event_processor<> > recorderAdapterEventProcessor;

    void sendInitEvents();

    virtual void process(boost::shared_ptr<NotificationFileInfo>) {};
};

#endif
