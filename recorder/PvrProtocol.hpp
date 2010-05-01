//
// PVR Protocol
//
// Copyright (C) Joachim Erbs, 2010
//

#ifndef PVR_PROTOCOL_HPP
#define PVR_PROTOCOL_HPP

extern "C"
{
#include <libavutil/avstring.h>
#include <libavformat/avformat.h>
}

class PvrProtocol
{
public:
    static void init();

private:
    PvrProtocol();
    ~PvrProtocol();

    static int pvrOpen(URLContext *h, const char *filename, int flags);
    static int pvrRead(URLContext *h, unsigned char *buf, int size);
    static int pvrWrite(URLContext *h, unsigned char *buf, int size);
    static int64_t pvrSeek(URLContext *h, int64_t pos, int whence);
    static int pvrClose(URLContext *h);

    static PvrProtocol* instance;

    URLProtocol m_prot;
};

class PvrContext
{
    friend class PvrProtocol;

    PvrContext(int fd)
	: m_fd(fd)
    {}

    int m_fd;
};

#endif
