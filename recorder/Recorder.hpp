//
// Recorder
//
// Copyright (C) Joachim Erbs, 2012
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

#ifndef RECORDER_HPP
#define RECORDER_HPP

#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"
#include "recorder/GeneralEvents.hpp"
#include "player/GeneralEvents.hpp"

#include <boost/shared_ptr.hpp>

#include <unistd.h>

class Recorder : public event_receiver<Recorder,
				       concurrent_queue<receive_fct_t, with_callback_function> >
{
    friend class event_processor<concurrent_queue<receive_fct_t, with_callback_function> >;

    boost::shared_ptr<event_processor<concurrent_queue<receive_fct_t, with_callback_function> > > m_event_processor;

    enum state_t {
	Closed,
	Opened
    };
    state_t m_state;

public:
    Recorder(event_processor_ptr_type evt_proc);
    ~Recorder();

    // Custom main loop for event_processor:
    void operator()();

private:
    static int interruptCallback(void* ptr);

    void process(boost::shared_ptr<RecorderInitEvent> event);

    void process(boost::shared_ptr<StartRecordingReq> event);
    void process(boost::shared_ptr<StopRecordingReq> event);

    void openPvrReader();
    void closePvrReader();
    void updateDuration();

    void notify();

    MediaRecorder* mediaRecorder;
    boost::shared_ptr<RecorderAdapter> recorderAdapter;

    AVFormatContext* m_avFormatContext;

    std::string m_tmpFile;

    int m_rfd;
    int m_wfd;
    int m_pipefd[2];
    int& m_piperfd;
    int& m_pipewfd;
};

#endif
