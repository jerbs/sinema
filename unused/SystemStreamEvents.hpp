//
// Media Player Events
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef SYSTEM_STREAM_EVENTS_HPP
#define SYSTEM_STREAM_EVENTS_HPP

struct SystemStreamChunkEvent
{
    SystemStreamChunkEvent(ssize_t size)
	: m_size(size),
	  m_buffer(new char[size])
    {
    }
    ~SystemStreamChunkEvent()
    {
	delete[](m_buffer);
    }

    ssize_t size() {return m_size;}
    void size(ssize_t size) {m_size = size;}
    char* buffer() {return m_buffer;}

private:
    ssize_t m_size;
    char* m_buffer;
};

struct SystemStreamGetMoreDataEvent
{
};

#endif
