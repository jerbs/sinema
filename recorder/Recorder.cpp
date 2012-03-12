//
// Recorder
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

#define _LARGEFILE64_SOURCE

#include "recorder/Recorder.hpp"
#include "recorder/RecorderAdapter.hpp"
#include "recorder/MediaRecorder.hpp"

#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

// #undef TRACE_DEBUG
// #define TRACE_DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

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
      m_avFormatContext(0),
      m_tmpFile("/tmp/tv.mpg"),
      m_rfd(-1),
      m_wfd(-1),
      m_piperfd(m_pipefd[0]),
      m_pipewfd(m_pipefd[1])
{
    if (pipe(m_pipefd) == -1)
    {
	TRACE_ERROR(<< "pipe failed: " << strerror(errno));
	exit(1);
    }

    if (fcntl(m_pipewfd, F_SETFL, O_NONBLOCK) == -1)
    {
	TRACE_ERROR(<< "fcntl failed: " << strerror(errno));
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
    if (m_avFormatContext != 0) closePvrReader();

    RecorderThreadNotification::setCallback(0);
}

void Recorder::process(boost::shared_ptr<RecorderInitEvent> event)
{
    TRACE_DEBUG(<< "tid = " << gettid());

    mediaRecorder = event->mediaRecorder;
    recorderAdapter = event->recorderAdapter;
}

void Recorder::process(boost::shared_ptr<StartRecordingReq> event)
{
    TRACE_DEBUG();

    int access = O_RDONLY | O_LARGEFILE;

#ifdef O_BINARY
    access |= O_BINARY;
#endif

    int error = 0;
    m_rfd = open(event->filename.c_str(), access, 0666);
    if (m_rfd == -1)
    {
	error = errno;
	TRACE_ERROR(<< "opening source \'" << event->filename << "\' failed: " << strerror(error));
    }
    else
    {
	access = O_CREAT | O_WRONLY | O_TRUNC | O_LARGEFILE;

#ifdef O_BINARY
	access |= O_BINARY;
#endif
	m_wfd = open(m_tmpFile.c_str(), access, 0666);
	if (m_wfd == -1)
	{
	    error = errno;
	    TRACE_ERROR(<< "opening target \'" << m_tmpFile << "\' failed: " << strerror(error));
	}
	else
	{
	    m_state = Opened;
	    // Too early to call openPvrReader.
	}
    }

    recorderAdapter->queue_event(boost::make_shared<StartRecordingResp>(m_tmpFile, error));
}

void Recorder::process(boost::shared_ptr<StopRecordingReq>)
{
    TRACE_DEBUG();

    int error = 0;
    if (m_rfd != -1)
    {
	int ret = close(m_rfd);
	if (ret == -1)
	{
	    error = errno;
	    TRACE_ERROR(<< "closing source failed: " << strerror(error));
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
	    TRACE_ERROR(<< "close target failed: " << strerror(error));
	}
	else
	{
	    m_wfd = -1;
	    closePvrReader();
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
	    TRACE_DEBUG(<< "start");

	    int res;
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

		res = poll(pfd, 2, -1);

		if (res < 0)
		{
		    if (res == EINTR)
		    {
			// ignore
		    }
		    else
		    {
			TRACE_THROW(std::string, "pipe failed (read)" << errno);
		    }
		}
		else if (res == 0)
		{
		    // timeout
		}
		else
		{
		    if (pfd[1].revents & POLLIN)
		    {
			bufferFill = read(m_rfd, &buffer[0], bufferSize);
			TRACE_DEBUG(<< "read " << bufferFill);
			if (bufferFill == -1)
			{
			    bufferFill = 0;
			    TRACE_ERROR(<< "read failed on m_rfd: " << strerror(errno));
			}
		    }
		    if (pfd[1].revents & POLLERR)
		    {
			TRACE_ERROR(<< "POLLERR on m_rfd.");
		    }
		    if (pfd[1].revents & POLLHUP)
		    {
			TRACE_ERROR(<< "POLLHUP on m_rfd.");
		    }
		}
	    }
	    else
	    {
		// Write data to storage:

		pfd[1].fd = m_wfd;
		pfd[1].events = POLLOUT;
		pfd[1].revents = 0;

		res = poll(pfd, 2, -1);

		if (res < 0)
		{
		    if (res == EINTR)
		    {
			// ignore
		    }
		    else
		    {
			TRACE_THROW(std::string, "pipe failed (read)" << errno);
		    }
		}
		else if (res == 0)
		{
		}
		else
		{
		    if (pfd[1].revents & POLLOUT)
		    {
			int num = write(m_wfd, &buffer[writePos], bufferFill);
			TRACE_DEBUG(<< "write " << num);
			if (num == -1)
			{
			    TRACE_ERROR(<< "write failed on m_wfd: " << strerror(errno));
			}
			else
			{
			    writePos += num;
			    bufferFill -= num;
			    
			    if (bufferFill == 0)
			    {
				writePos = 0;
			    }
			    
			    updateDuration();
			}
		    }
		    if (pfd[1].revents & POLLERR)
		    {
			TRACE_ERROR(<< "POLLERR on m_wfd.");
		    }
		    if (pfd[1].revents & POLLHUP)
		    {
			TRACE_ERROR(<< "POLLHUP on m_wfd.");
		    }
		}
	    }

	    if (res > 0 && pfd[0].revents)
	    {
		// Read data from pipe:

		if (pfd[0].revents & POLLIN)
		{
		    TRACE_DEBUG(<< "reading pipe");
		    // Read dummy data:
		    int bufSize = 0x1000;
		    char buf[bufSize];
		    int num = read(m_piperfd, buf, bufSize);
		    if (num == -1)
		    {
			TRACE_ERROR(<< "read failed on m_piperfd: " << strerror(errno));
		    }
		}
		if (pfd[0].revents & POLLERR)
		{
		    TRACE_ERROR(<< "POLLERR on m_piperfd.");
		}
		if (pfd[0].revents & POLLHUP)
		{
		    TRACE_ERROR(<< "POLLHUP on m_piperfd.");
		}
	    }

	    TRACE_DEBUG(<< "end");
	}
	else
	{
            m_event_processor->dequeue_and_process();
        }
    }
}

void Recorder::openPvrReader()
{
    std::string url = "sto:"+m_tmpFile;

    if (m_avFormatContext)
    {
	closePvrReader();
    }

    int ret = avformat_open_input(&m_avFormatContext,
				  url.c_str(),
				  0,   // don't force any format, AVInputFormat*,
				  0);  // AVDictionary**
    if (ret != 0)
    {
	TRACE_ERROR(<< "avformat_open_input failed: " << ret);
	m_avFormatContext = 0;
	return;
    }
}

void Recorder::closePvrReader()
{
    if (m_avFormatContext)
    {
	av_close_input_file(m_avFormatContext);
	m_avFormatContext = 0;
    }
}

void Recorder::updateDuration()
{
    TRACE_DEBUG();
    return;
    if (m_avFormatContext)
    {
	openPvrReader();
	int ret = av_find_stream_info(m_avFormatContext);
	if (ret < 0)
	{
	    TRACE_ERROR(<< "av_find_stream_info failed: " << ret);
	    av_close_input_file(m_avFormatContext);
	    m_avFormatContext = 0;
	    return;
	}

	boost::shared_ptr<NotificationFileInfo> nfi(new NotificationFileInfo());
	const double INV_AV_TIME_BASE = double(1)/AV_TIME_BASE;
	nfi->fileName = m_tmpFile;
	nfi->duration = double(m_avFormatContext->duration) * INV_AV_TIME_BASE;
	nfi->file_size = m_avFormatContext->file_size;
	TRACE_DEBUG(<< "NotificationFileInfo = " << *nfi);
	mediaRecorder->queue_event(nfi);
    }
    else
    {
	openPvrReader();
    }

}

void Recorder::notify()
{
    TRACE_DEBUG();
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
	    TRACE_ERROR(<< "write m_pipewfd failed: " << strerror(errno));
	}
    }
}
