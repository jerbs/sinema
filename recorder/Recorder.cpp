//
// Recorder
//
// Copyright (C) Joachim Erbs
//

#define _LARGEFILE64_SOURCE

#include "recorder/Recorder.hpp"
#include "recorder/RecorderAdapter.hpp"

#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

// #undef DEBUG
// #define DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

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

Recorder* Recorder::instance = 0;

Recorder::Recorder(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      m_event_processor(evt_proc),
      m_state(Closed),
      mediaRecorder(0),
      m_rfd(-1),
      m_wfd(-1),
      m_piperfd(m_pipefd[0]),
      m_pipewfd(m_pipefd[1])
{
    if (pipe(m_pipefd) == -1)
    {
	ERROR(<< "pipe failed: " << strerror(errno));
	exit(1);
    }

    if (fcntl(m_pipewfd, F_SETFL, O_NONBLOCK) == -1)
    {
	ERROR(<< "fcntl failed: " << strerror(errno));
	exit(1);
    }

    // FIXME: This only works with a single Recorder instance.
    instance = this;
    RecorderThreadNotification::setCallback(notify);
}

Recorder::~Recorder()
{
    if (m_piperfd != -1) close(m_piperfd);
    if (m_pipewfd != -1) close(m_pipewfd);

    RecorderThreadNotification::setCallback(0);
}

void Recorder::process(boost::shared_ptr<RecorderInitEvent> event)
{
    DEBUG();

    mediaRecorder = event->mediaRecorder;
    recorderAdapter = event->recorderAdapter;
}

void Recorder::process(boost::shared_ptr<StartRecordingReq> event)
{
    DEBUG();

    const char* tmpFile = 0;

    int access = O_RDONLY | O_LARGEFILE;

#ifdef O_BINARY
    access |= O_BINARY;
#endif

    int error = 0;
    m_rfd = open(event->filename.c_str(), access, 0666);
    if (m_rfd == -1)
    {
	error = errno;
	ERROR(<< "opening source failed: " << strerror(error));
    }
    else
    {
	access = O_CREAT | O_WRONLY | O_TRUNC | O_LARGEFILE;

#ifdef O_BINARY
	access |= O_BINARY;
#endif
	tmpFile = "/tmp/tv.mpg";
	m_wfd = open(tmpFile, access, 0666);
	if (m_wfd == -1)
	{
	    error = errno;
	    ERROR(<< "opening target failed: " << strerror(error));
	}
	else
	{
	    m_state = Opened;
	}
    }

    recorderAdapter->queue_event(boost::make_shared<StartRecordingResp>(tmpFile, error));
}

void Recorder::process(boost::shared_ptr<StopRecordingReq> event)
{
    DEBUG();

    int error = 0;
    if (m_rfd != -1)
    {
	int ret = close(m_rfd);
	if (ret == -1)
	{
	    error = errno;
	    ERROR(<< "closing source failed: " << strerror(error));
	}
	else
	{
	    m_rfd = -1;
	}
    }
    if (m_wfd != -1)
    {
	int ret = close(m_wfd);
	if (ret == -1)
	{
	    error = errno;
	    ERROR(<< "close target failed: " << strerror(error));
	}
	else
	{
	    m_wfd = -1;
	}
    }

    m_state = Closed;

    recorderAdapter->queue_event(boost::make_shared<StopRecordingResp>(error));
}

void Recorder::operator()()
{
    const int bufferSize = 0x10000;    // 64kB
    // const int bufferSize = 0x100000;  // 1MB
    int bufferFill = 0;    // Number of bytes in the buffer.
    char buffer[bufferSize];
    int writePos = 0;

    while(!m_event_processor->terminating())
    {
	if (m_event_processor->empty() && m_state == Opened)
	{
	    DEBUG(<< "start");

	    pollfd pfd[2];

	    pfd[0].fd = m_piperfd;
	    pfd[0].events = POLLIN;
	    pfd[0].revents = 0;

	    if (bufferFill == 0)
	    {
		// Read new data from device:

		pfd[1].fd = m_rfd;
		pfd[1].events = POLLIN;
		pfd[1].revents = 0;

		int res = poll(pfd, 2, -1);

		if (pfd[1].revents & POLLIN)
		{
		    bufferFill = read(m_rfd, &buffer[0], bufferSize);
		    DEBUG(<< "read " << bufferFill);
		    if (bufferFill == -1)
		    {
			bufferFill = 0;
			ERROR(<< "read failed on m_rfd: " << strerror(errno));
		    }
		}
		if (pfd[1].revents & POLLERR)
		{
		    ERROR(<< "POLLERR on m_rfd.");
		}
		if (pfd[1].revents & POLLHUP)
		{
		    ERROR(<< "POLLHUP on m_rfd.");
		}
	    }
	    else
	    {
		// Write data to storage:

		pfd[1].fd = m_wfd;
		pfd[1].events = POLLOUT;
		pfd[1].revents = 0;

		int res = poll(pfd, 2, -1);

		if (pfd[1].revents & POLLOUT)
		{
		    int num = write(m_wfd, &buffer[writePos], bufferFill);
		    DEBUG(<< "write " << num);
		    if (num == -1)
		    {
			ERROR(<< "write failed on m_wfd: " << strerror(errno));
		    }
		    else
		    {
			writePos += num;
			bufferFill -= num;

			if (bufferFill == 0)
			{
			    writePos = 0;
			}
		    }
		}
		if (pfd[1].revents & POLLERR)
		{
		    ERROR(<< "POLLERR on m_wfd.");
		}
		if (pfd[1].revents & POLLHUP)
		{
		    ERROR(<< "POLLHUP on m_wfd.");
		}
	    }



	    if (pfd[0].revents)
	    {
		// Read data from pipe:

		if (pfd[0].revents & POLLIN)
		{
		    DEBUG(<< "reading pipe");
		    // Read dummy data:
		    int bufSize = 0x1000;
		    char buf[bufSize];
		    int num = read(m_piperfd, buf, bufSize);
		    if (num == -1)
		    {
			ERROR(<< "read failed on m_piperfd: " << strerror(errno));
		    }
		}
		if (pfd[0].revents & POLLERR)
		{
		    ERROR(<< "POLLERR on m_piperfd.");
		}
		if (pfd[0].revents & POLLHUP)
		{
		    ERROR(<< "POLLHUP on m_piperfd.");
		}
	    }

	    DEBUG(<< "end");
	}
	else
	{
            m_event_processor->dequeue_and_process();
        }
    }
}

void Recorder::notify()
{
    DEBUG();
    const int bufferSize = 4;
    char buffer[bufferSize] = {0,0,0,0};
    int num = write(instance->m_pipewfd, buffer, bufferSize);
    if (num == -1)
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    // OK: May happen when Recorder gets many messages and stays 
	    //     in Closed state.
	}
	else
	{
	    ERROR(<< "write m_pipewfd failed: " << strerror(errno));
	}
    }
}
