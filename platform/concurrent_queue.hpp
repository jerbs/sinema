//
// Thread-Safe multiple Producer, multiple Consumer Queue
//
// Copyright (C) Anthony Williams, 2008
// Copyright (C) Joachim Erbs, 2009-2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
// For details about the implementation see
// http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
//

#ifndef CONCURRENT_QUEUE_HPP
#define CONCURRENT_QUEUE_HPP

#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <queue>

class without_callback_function
{
public:
    typedef boost::function<void ()> notification_function_type;

protected:
    void notify() {}
};

class with_callback_function
{
public:
    typedef boost::function<void ()> notification_function_type;

    void attach(notification_function_type fct) {m_callback = fct;}

protected:
    void notify()
    {
	if (m_callback)
	{
	    m_callback();
	}
    }

private:
    notification_function_type m_callback;
};

class NoTrigger{};

template<typename T, class base_type = without_callback_function>
class concurrent_queue : public base_type
{
private:
    std::queue<T> m_queue;
    mutable boost::mutex m_mutex;
    boost::condition_variable m_condition_variable;

public:
    void push(T const& data)
    {
	boost::mutex::scoped_lock lock(m_mutex);
	m_queue.push(data);
	lock.unlock();
        m_condition_variable.notify_one();
	this->notify();
    }

    bool empty() const
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_queue.empty();
    }

    bool try_pop(T& popped_value)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        if(m_queue.empty())
        {
            return false;
        }
        
        popped_value=m_queue.front();
        m_queue.pop();
        return true;
    }

    void wait_and_pop(T& popped_value)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        while(m_queue.empty())
        {
            m_condition_variable.wait(lock);
        }
        
        popped_value=m_queue.front();
        m_queue.pop();
    }
};

#endif
