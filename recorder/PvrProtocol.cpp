//
// PVR Protocol
//
// Copyright (C) Joachim Erbs, 2010
//

#define _LARGEFILE64_SOURCE

#include "recorder/PvrProtocol.hpp"
#include "platform/Logging.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>

#undef DEBUG
#define DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

// -------------------------------------------------------------------
// Callbacks used by FFmpeg:

int PvrProtocol::pvrOpen(URLContext *h, const char *filename, int flags)
{
    DEBUG(<< filename);

    int access;
    int fd;

    av_strstart(filename, "pvr:", &filename);

    if (flags & URL_RDWR)
    {
        access = O_CREAT | O_TRUNC | O_RDWR;
    }
    else if (flags & URL_WRONLY)
    {
        access = O_CREAT | O_TRUNC | O_WRONLY;
    } else
    {
        access = O_RDONLY;
    }
#ifdef O_BINARY
    access |= O_BINARY;
#endif

    fd = open(filename, access, 0666);
    if (fd == -1)
        return AVERROR(errno);

    PvrContext* context = new PvrContext(fd);
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
    PvrContext* context = (PvrContext*)(h->priv_data);
    int fd = context->m_fd;
    delete(context);
    h->priv_data = 0;
    
    return close(fd);
}

// -------------------------------------------------------------------

static void show_protocols(void)
{
    URLProtocol *up = 0;
    
    DEBUG(<< "FFmpeg protocols:");
    while((up = av_protocol_next(up)))
	DEBUG(<< up->name);
}

PvrProtocol* PvrProtocol::instance = 0;

PvrProtocol::PvrProtocol()
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

void PvrProtocol::init()
{
    if (instance == 0)
    {
	instance = new PvrProtocol();
    }
}
