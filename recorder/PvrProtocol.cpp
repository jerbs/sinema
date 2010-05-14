//
// PVR Protocol
//
// Copyright (C) Joachim Erbs
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

//#undef DEBUG
//#define DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

PvrProtocol* PvrProtocol::instance = 0;

static void show_protocols(void)
{
    URLProtocol *up = 0;
    
    DEBUG(<< "FFmpeg protocols:");
    while((up = av_protocol_next(up)))
	DEBUG(<< up->name);
}

// -------------------------------------------------------------------

PvrProtocol::PvrProtocol(boost::shared_ptr<RecorderAdapter> recorderAdapter)
    : recorderAdapter(recorderAdapter)
{
    m_prot.name = "pvr";
    m_prot.url_open = pvrOpen;
    m_prot.url_read = pvrRead;
    m_prot.url_write = pvrWrite;
    m_prot.url_seek = pvrSeek;
    m_prot.url_close = pvrClose;
    m_prot.next = 0;
    m_prot.url_read_pause = 0;
    m_prot.url_read_seek = 0;

    av_register_protocol(&m_prot);

    show_protocols();
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
}

// -------------------------------------------------------------------
// Callbacks used by FFmpeg:

int PvrProtocol::pvrOpen(URLContext *h, const char *filename, int flags)
{
    DEBUG(<< filename);

    av_strstart(filename, "pvr:", &filename);

    boost::promise<boost::shared_ptr<StartRecordingResp> > promise;
    boost::unique_future<boost::shared_ptr<StartRecordingResp> > future = promise.get_future();
    boost::shared_ptr<StartRecordingReq> req(new StartRecordingReq(filename));
    boost::shared_ptr<StartRecordingSReq> sreq(new StartRecordingSReq(req, promise));

    instance->recorderAdapter->queue_event(sreq);
   
    future.wait();

    boost::shared_ptr<StartRecordingResp> resp = future.get();

    if (resp->error)
    {
	DEBUG(<< "open failed: " << strerror(resp->error));
        return AVERROR(resp->error);
    }

    PvrContext* context = new PvrContext(resp->fd);
    h->priv_data = (void *)context;
    return 0;
}

int PvrProtocol::pvrRead(URLContext *h, unsigned char *buf, int size)
{
    DEBUG();
    PvrContext* context = (PvrContext*)(h->priv_data);
    int& fd = context->m_fd;
    return read(fd, buf, size);
}

int PvrProtocol::pvrWrite(URLContext *h, unsigned char *buf, int size)
{
    DEBUG();
    PvrContext* context = (PvrContext*)(h->priv_data);
    int& fd = context->m_fd;
    return write(fd, buf, size);
}

int64_t PvrProtocol::pvrSeek(URLContext *h, int64_t pos, int whence)
{
    DEBUG();
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

int PvrProtocol::pvrClose(URLContext *h)
{
    DEBUG();

    boost::promise<boost::shared_ptr<StopRecordingResp> > promise;
    boost::unique_future<boost::shared_ptr<StopRecordingResp> > future = promise.get_future();
    boost::shared_ptr<StopRecordingReq> req(new StopRecordingReq());
    boost::shared_ptr<StopRecordingSReq> sreq(new StopRecordingSReq(req, promise));

    instance->recorderAdapter->queue_event(sreq);
   
    future.wait();

    boost::shared_ptr<StopRecordingResp> resp = future.get();

    if (resp->error)
    {
	DEBUG(<< "close failed: " << strerror(resp->error));
        return AVERROR(resp->error);
    }

    PvrContext* context = (PvrContext*)(h->priv_data);
    // int fd = context->m_fd;
    delete(context);
    h->priv_data = 0;
    
    // return close(fd);
    return 0;
}

// -------------------------------------------------------------------

