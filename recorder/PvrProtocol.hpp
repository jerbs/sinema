//
// PVR Protocol
//
// Copyright (C) Joachim Erbs
//

#ifndef PVR_PROTOCOL_HPP
#define PVR_PROTOCOL_HPP

#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"
#include "recorder/GeneralEvents.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/thread/future.hpp>

extern "C"
{
#include <libavutil/avstring.h>
#include <libavformat/avformat.h>
}

class PvrStorage;
class RecorderAdapter;

class StorageProtocol
{
public:
    static void init();

protected:
    StorageProtocol();
    ~StorageProtocol();

    static int pvrOpen(URLContext *h, const char *filename, int flags);
    static int pvrRead(URLContext *h, unsigned char *buf, int size);
    static int pvrWrite(URLContext *h, unsigned char *buf, int size);
    static int64_t pvrSeek(URLContext *h, int64_t pos, int whence);
    static int pvrClose(URLContext *h);

private:
    static StorageProtocol* instance;

    URLProtocol m_prot;
};

class PvrProtocol : public StorageProtocol
{
public:
    static void init(boost::shared_ptr<RecorderAdapter> recorderAdapter);

protected:
    PvrProtocol(boost::shared_ptr<RecorderAdapter> recorderAdapter);
    ~PvrProtocol();

    static int pvrOpen(URLContext *h, const char *filename, int flags);
    static int pvrRead(URLContext *h, unsigned char *buf, int size);
    static int pvrClose(URLContext *h);

private:
    static PvrProtocol* instance;

    URLProtocol m_prot;
    boost::shared_ptr<RecorderAdapter> recorderAdapter;
   
};

class PvrContext
{
    friend class StorageProtocol;

    PvrContext(int fd)
	: m_fd(fd)
    {}

    int m_fd;
};

#endif
