//
// PVR Protocol
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

#include "recorder/PvrProtocol.hpp"
#include "recorder/RecorderAdapter.hpp"
#include "platform/Logging.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

//#undef TRACE_DEBUG
//#define TRACE_DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;


PvrProtocol* PvrProtocol::instance = 0;
StorageProtocol* StorageProtocol::instance = 0;

static void show_protocols(void)
{
    void *opaque = 0;
    const char* name;

    TRACE_DEBUG(<< "FFmpeg output protocols:");
    while((name = avio_enum_protocols(&opaque, 1)))
	TRACE_DEBUG(<< name);

    TRACE_DEBUG(<< "FFmpeg input protocols:");
    while((name = avio_enum_protocols(&opaque, 0)))
	TRACE_DEBUG(<< name);
}

// -------------------------------------------------------------------

StorageProtocol::StorageProtocol()
{
}

StorageProtocol::~StorageProtocol()
{
}

void StorageProtocol::init()
{
    if (instance == 0)
    {
	instance = new StorageProtocol();
	
    }

    static  URLProtocol prot;
    prot.name = "sto";
    prot.url_open = pvrOpen;
    prot.url_read = pvrRead;
    prot.url_write = pvrWrite;
    prot.url_seek = pvrSeek;
    prot.url_close = pvrClose;
    prot.next = 0;
    prot.url_read_pause = 0;
    prot.url_read_seek = 0;

    ffurl_register_protocol(&prot, sizeof(prot));

    show_protocols();
}

// -------------------------------------------------------------------
// Callbacks used by FFmpeg:

int StorageProtocol::pvrOpen(URLContext *h, const char *filename, int /* flags */)
{
    TRACE_DEBUG(<< filename);

    av_strstart(filename, "sto:", &filename);

    std::string fileName(filename);

    int access = O_RDONLY;
#ifdef O_BINARY
    access |= O_BINARY;
#endif

    int fd = open(fileName.c_str(), access, 0666);
    if (fd == -1)
        return AVERROR(errno);

    PvrContext* context = new PvrContext(fd);
    h->priv_data = (void *)context;
    return 0;
}

int StorageProtocol::pvrRead(URLContext *h, unsigned char *buf, int size)
{
    TRACE_DEBUG();
    PvrContext* context = (PvrContext*)(h->priv_data);
    int& fd = context->m_fd;
    int num = read(fd, buf, size);
    TRACE_DEBUG(<< num);
    return num;
}

int StorageProtocol::pvrWrite(URLContext *h, unsigned char *buf, int size)
{
    return pvrWrite(h, (const unsigned char*)buf, size);
}

int StorageProtocol::pvrWrite(URLContext *h, const unsigned char *buf, int size)
{
    TRACE_DEBUG();
    PvrContext* context = (PvrContext*)(h->priv_data);
    int& fd = context->m_fd;
    return write(fd, buf, size);
}

int64_t StorageProtocol::pvrSeek(URLContext *h, int64_t pos, int whence)
{
    TRACE_DEBUG( << "pos=" << pos << ", whence=" << whence);
    PvrContext* context = (PvrContext*)(h->priv_data);
    int& fd = context->m_fd;

    if (whence == AVSEEK_SIZE)
    {
        struct stat st;
        int ret = fstat(fd, &st);
        return ret < 0 ? AVERROR(errno) : st.st_size;
    }

    return lseek64(fd, pos, whence);
}

int StorageProtocol::pvrClose(URLContext *h)
{
    TRACE_DEBUG();

    PvrContext* context = (PvrContext*)(h->priv_data);
    int fd = context->m_fd;
    delete(context);
    h->priv_data = 0;
    
    return close(fd);
}

// -------------------------------------------------------------------

PvrProtocol::PvrProtocol(boost::shared_ptr<RecorderAdapter> recorderAdapter)
    : recorderAdapter(recorderAdapter)
{
}

PvrProtocol::~PvrProtocol()
{
}

void PvrProtocol::init(boost::shared_ptr<RecorderAdapter> recorderAdapter)
{
    if (instance == 0)
    {
	instance = new PvrProtocol(recorderAdapter);
    }

    static  URLProtocol prot;
    prot.name = "pvr";
    prot.url_open = pvrOpen;
    prot.url_read = pvrRead;
    prot.url_write = pvrWrite;
    prot.url_seek = pvrSeek;
    prot.url_close = pvrClose;
    prot.next = 0;
    prot.url_read_pause = 0;
    prot.url_read_seek = 0;

    ffurl_register_protocol(&prot, sizeof(prot));

    show_protocols();
}

// -------------------------------------------------------------------
// Callbacks used by FFmpeg:

int PvrProtocol::pvrOpen(URLContext *h, const char *filename, int flags)
{
    TRACE_DEBUG(<< filename);

    av_strstart(filename, "pvr:", &filename);

    std::string fileName(filename);

    boost::promise<boost::shared_ptr<StartRecordingResp> > promise;
    boost::unique_future<boost::shared_ptr<StartRecordingResp> > future = promise.get_future();
    boost::shared_ptr<StartRecordingReq> req(new StartRecordingReq(fileName));
    boost::shared_ptr<StartRecordingSReq> sreq(new StartRecordingSReq(req, std::move(promise)));

    instance->recorderAdapter->queue_event(sreq);
   
    future.wait();

    boost::shared_ptr<StartRecordingResp> resp = future.get();

    if (resp->error)
    {
	TRACE_DEBUG(<< "open failed: " << strerror(resp->error));
	return AVERROR(resp->error);
    }

    TRACE_DEBUG(<< resp->tempFilename);

    return StorageProtocol::pvrOpen(h, resp->tempFilename.c_str(), flags);
}

int PvrProtocol::pvrRead(URLContext *h, unsigned char *buf, int size)
{
    TRACE_DEBUG();
    int n = 0;
    while(1)
    {
	// FIXME: End of file detection is needed here.
	int num = StorageProtocol::pvrRead(h, buf, size);
	if (num == 0)
	{
	    n++;
	    if (n == 100)
	    {
		TRACE_DEBUG(<< "waiting 1 second");
		n = 0;
	    }
	    usleep(10*1000); // 10 milli seconds
	}
	else
	{
	    return num;
	}
    }
}

int PvrProtocol::pvrClose(URLContext *h)
{
    TRACE_DEBUG();

    boost::promise<boost::shared_ptr<StopRecordingResp> > promise;
    boost::unique_future<boost::shared_ptr<StopRecordingResp> > future = promise.get_future();
    boost::shared_ptr<StopRecordingReq> req(new StopRecordingReq());
    boost::shared_ptr<StopRecordingSReq> sreq(new StopRecordingSReq(req, std::move(promise)));

    instance->recorderAdapter->queue_event(sreq);

    future.wait();

    boost::shared_ptr<StopRecordingResp> resp = future.get();

    if (resp->error)
    {
	TRACE_DEBUG(<< "close failed: " << strerror(resp->error));
	return AVERROR(resp->error);
    }

    return StorageProtocol::pvrClose(h);
}

// -------------------------------------------------------------------

