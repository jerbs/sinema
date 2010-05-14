//
// Recorder
//
// Copyright (C) Joachim Erbs
//

#include "recorder/Recorder.hpp"
#include "recorder/RecorderAdapter.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

// ===================================================================

RecorderThreadNotification::RecorderThreadNotification()
{
    // Here the Recorder thread is triggered to leave the select() call.
    if (m_fct)
    {
        m_fct();
    }
}

void RecorderThreadNotification::setCallback(fct_t fct)
{
    m_fct = fct;
}

RecorderThreadNotification::fct_t RecorderThreadNotification::m_fct;

// ===================================================================

void Recorder::process(boost::shared_ptr<RecorderInitEvent> event)
{
    DEBUG();

    mediaRecorder = event->mediaRecorder;
    recorderAdapter = event->recorderAdapter;
}

void Recorder::process(boost::shared_ptr<StartRecordingReq> event)
{
    DEBUG();

    int access = O_RDONLY;

#ifdef O_BINARY
    access |= O_BINARY;
#endif

    int error = 0;
    fd = open(event->filename.c_str(), access, 0666);
    if (fd == -1)
    {
	error = errno;
	DEBUG(<< "open failed: " << strerror(error));
    }

    recorderAdapter->queue_event(boost::make_shared<StartRecordingResp>("", fd, error));
}

void Recorder::process(boost::shared_ptr<StopRecordingReq> event)
{
    DEBUG();

    int error = 0;
    int ret = close(fd);
    if (ret == -1)
    {
	error = errno;
	DEBUG(<< "close failed: " << strerror(error));
    }
    else
    {
	fd = -1;
    }
    recorderAdapter->queue_event(boost::make_shared<StopRecordingResp>(error));
}

void Recorder::operator()()
{
    while(!m_event_processor->terminating())
    {
	if (0)
	{
	    if (!m_event_processor->empty())
            {
                m_event_processor->dequeue_and_process();
            }
	}
	else
	{
            m_event_processor->dequeue_and_process();
        }

    }
}
