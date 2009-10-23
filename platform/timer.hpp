//
// Timer Service
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef TIMER_HPP
#define TIMER_HPP

#include <boost/function.hpp>
#include <strings.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>   // perror

template<class concurrent_queue>
class event_processor;

typedef struct timespec timespec_t;

class timer : private boost::noncopyable
{
    template<class concurrent_queue>
    friend class event_processor;

    typedef boost::function<void ()> timeout_fct_t;
    typedef struct timespec timespec_t;
    typedef struct itimerspec itimerspec_t;

public:
    timer()
	: m_clockid(CLOCK_REALTIME),
	  m_timeout_fct()
    {
	sigevent_t event;
	bzero((char*)&event, sizeof(event));
	event.sigev_notify = SIGEV_THREAD;
	event._sigev_un._sigev_thread._function = timeout_handler;
	event._sigev_un._sigev_thread._attribute = NULL;
	event.sigev_value.sival_ptr = (void*)this;

	int ret = timer_create(m_clockid, &event, &m_timerid);

	if (ret != 0)
	{
	    perror("timer_create failed");
	    exit(-1);
	}

	m_flags = 0;
	bzero((char*)&m_timerspec, sizeof(m_timerspec));
    }

    ~timer()
    {
	int ret = timer_delete(m_timerid);
	if (ret != 0)
	{
	    perror("timer_delete failed");
	    exit(-1);
	}
    }

    timer& absolute(timespec_t abs)
    {
	m_flags = TIMER_ABSTIME;
	m_timerspec.it_value = abs;
	return *this;
    }

    timer& relative(timespec_t abs)
    {
	m_flags = 0;
	m_timerspec.it_value = abs;
	return *this;
    }

    timer& periodic(timespec_t per)
    {
	m_timerspec.it_interval = per;
	return *this;
    }

    int get_overrun()
    {
	int ret = timer_getoverrun(m_timerid);
	if (ret < 0)
	{
	    perror("timer_getoverrun failed");
	    exit(-1);
	}
	return ret;
    }

    // Returns amount of time until the timer expires:
    timespec_t get_remaining_time()
    {
	struct itimerspec ts;
	int ret = timer_gettime(m_timerid, &ts);
	if (ret != 0)
	{
	    perror("timer_gettime failed");
	    exit(-1);
	}
	return ts.it_value;
    }

    // Retrieves absolute time of clock used by timer:
    timespec_t get_current_time()
    {
	struct timespec res;
	int ret = clock_gettime(m_clockid, &res);
	if (ret != 0)
	{
	    perror("clock_gettime failed");
	    exit(-1);
	}
	return res;
    }

private:
    clockid_t m_clockid;
    timer_t m_timerid;
    int m_flags;
    itimerspec_t m_timerspec;
    timeout_fct_t m_timeout_fct;

    void start_timer(timeout_fct_t& fct)
    {
	m_timeout_fct = fct;
	int ret = timer_settime(m_timerid, m_flags, &m_timerspec, NULL);
	if (ret != 0)
	{
	    perror("timer_settime failed");
	    exit(-1);
	}

    }

    void stop_timer()
    {
	itimerspec_t ts;
	bzero((char*)&ts, sizeof(ts));
	int ret = timer_settime(m_timerid, 0, &ts, NULL);
	if (ret != 0)
	{
	    perror("stop_timer: timer_settime failed");
	    exit(-1);
	}

	// man timer_settime: The effect of disarming or resetting a 
	// timer with pending expiration notifications is unspecified.
    }

    static void timeout_handler(sigval_t sigval)
    {
	timer* obj = (timer*)sigval.sival_ptr;
	obj->timeout();
    }

    inline void timeout()
    {
	m_timeout_fct();
    }
};

inline double getSeconds(const timespec_t& t)
{
    double d = t.tv_sec + double(t.tv_nsec) / double(1000*1000*1000);
    return d;
}

inline timespec_t getTimespec(double seconds)
{
    timespec_t t;
    t.tv_sec = seconds;
    t.tv_nsec = (seconds-double(t.tv_sec)) * 1000*1000*1000;
    return t;
}

inline timespec_t operator+(const timespec_t& t1, const timespec_t& t2)
{
    timespec_t td;

    td.tv_sec  = t1.tv_sec  + t2.tv_sec;
    td.tv_nsec = t1.tv_nsec + t2.tv_nsec;
    if (td.tv_nsec > 1000*1000*1000)
    {
	td.tv_sec  += 1;
	td.tv_nsec -= 1000*1000*1000;
    }

    return td;
}

inline timespec_t operator-(const timespec_t& t1, const timespec_t& t2)
{
    timespec_t td;

    td.tv_sec  = t1.tv_sec  - t2.tv_sec;
    td.tv_nsec = t1.tv_nsec - t2.tv_nsec;
    if (td.tv_nsec < 0)
    {
	td.tv_sec  -= 1;
	td.tv_nsec += 1000*1000*1000;
    }

    return td;
}

inline bool operator>(const timespec_t& t1, const timespec_t& t2)
{
    if (t1.tv_sec > t2.tv_sec)
    {
	return true;
    }
    else if (t1.tv_sec == t2.tv_sec &&
	     t1.tv_nsec > t2.tv_nsec)
    {
	return true;
    }
    else
    {
	return false;
    }
}

inline bool operator<(const timespec_t& t1, const timespec_t& t2)
{
    return t2 > t1;
}

inline bool operator==(const timespec_t& t1, const timespec_t& t2)
{
    return (t1.tv_sec == t2.tv_sec) && (t1.tv_nsec == t2.tv_nsec);
}

inline bool operator!=(const timespec_t& t1, const timespec_t& t2)
{
    return ! (t1 == t2);
}

inline std::ostream& operator<<(std::ostream& strm, timespec_t t)
{
    strm << "(" << t.tv_sec << "," << t.tv_nsec << ")";
    return strm;
}

#endif
