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
    static int pvrWrite(URLContext *h, const unsigned char *buf, int size);
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
