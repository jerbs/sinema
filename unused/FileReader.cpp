//
// File Reader
//
// Copyright (C) Joachim Erbs, 2009
//

#include "player/FileReader.hpp"
#include "player/Demuxer.hpp"

#include <boost/make_shared.hpp>

FileReader::FileReader(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      m_fd(m_aiocb.aio_fildes),
      m_offset(m_aiocb.aio_offset),
      m_buffer(m_aiocb.aio_buf),
      m_chunkEvent(0),
      m_maxChunkSize(0x100000), // 1MB
      m_demuxer()
{
    bzero( (char *)&m_aiocb, sizeof(m_aiocb) );
    m_aiocb.aio_fildes = -1;
    m_aiocb.aio_nbytes = m_maxChunkSize;
    m_aiocb.aio_offset = 0;
    m_aiocb.aio_buf = 0;

    // Callback notification:
    m_aiocb.aio_sigevent.sigev_notify = SIGEV_THREAD;
    m_aiocb.aio_sigevent._sigev_un._sigev_thread._function = aioCompletionHandler;
    m_aiocb.aio_sigevent._sigev_un._sigev_thread._attribute = NULL;
    m_aiocb.aio_sigevent.sigev_value.sival_ptr = this;
 
}

FileReader::~FileReader()
{
    if (m_fd != -1)
	close(m_fd);

    if (m_chunkEvent)
	delete(m_chunkEvent);
}

void FileReader::process(boost::shared_ptr<InitEvent> event)
{
    TRACE_DEBUG();
    m_demuxer = event->demuxer;
}

void FileReader::process(boost::shared_ptr<OpenFileReq> event)
{
    TRACE_DEBUG(<< event->fileName);

    m_fd = open(event->fileName.c_str(), O_RDONLY);
    if (m_fd < 0)
    {
	perror("open");
	return;
    }

    m_offset = 0;
}

void FileReader::process(boost::shared_ptr<CloseFileReq> event)
{
    TRACE_DEBUG();
    close(m_fd);
    m_fd = -1;
    m_offset = 0;
}

void FileReader::process(boost::shared_ptr<SystemStreamGetMoreDataEvent> event)
{
    // TRACE_DEBUG();
    if (m_fd < 0)
    {
	return;
    }

    m_chunkEvent = new SystemStreamChunkEvent(m_maxChunkSize);
    m_buffer = m_chunkEvent->buffer();

    int ret = aio_read(&m_aiocb);
    if (ret < 0)
    {
	perror("aio_read");
    }
}

void FileReader::aioCompletionHandler(sigval_t sigval)
{
    FileReader* obj = (FileReader*)sigval.sival_ptr;
    obj->completed(sigval);
}

void FileReader::completed(sigval_t& sigval)
{
    // TRACE_DEBUG();
    if (aio_error(&m_aiocb) == 0)
    {
	// Request completed successfully
	int ret = aio_return(&m_aiocb);

	if (ret > 0)
	{
	    // Read data chunk:
	    m_offset += ret;
	    m_buffer = 0;
	    m_chunkEvent->size(ret);
	    boost::shared_ptr<SystemStreamChunkEvent> chunkEvent(m_chunkEvent);
	    m_demuxer->queue_event(chunkEvent);
	}
	else
	{
	    if (ret < 0)
	    {
		// Error occurred:
		TRACE_ERROR(<< "aio_return: " << ret);
	    }
	    else  // ret == 0
	    {
		// End of file
	    }

	    delete(m_chunkEvent);
	    m_chunkEvent = 0;
	}
    }
}
