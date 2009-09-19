//
// File Reader
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef FILE_READER_HPP
#define FILE_READER_HPP

#include "player/GeneralEvents.hpp"
#include "player/SystemStreamEvents.hpp"
#include "platform/event_receiver.hpp"

#include <aio.h>
#include <string>
#include <boost/shared_ptr.hpp>

class FileReader : public event_receiver<FileReader>
{
    friend class event_processor;
    typedef struct aiocb aiocb_t;

public:
    FileReader(event_processor_ptr_type evt_proc);
    ~FileReader();

private:
    aiocb_t m_aiocb;
    int& m_fd;
    off_t& m_offset;
    volatile void*& m_buffer;
    SystemStreamChunkEvent* m_chunkEvent;
    int m_maxChunkSize;
    boost::shared_ptr<Demuxer> m_demuxer;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<OpenFileEvent> event);
    void process(boost::shared_ptr<CloseFileEvent> event);
    void process(boost::shared_ptr<SystemStreamGetMoreDataEvent> event);

    static void aioCompletionHandler(sigval_t sigval);
    void completed(sigval_t& sigval);
};

#endif
