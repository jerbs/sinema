//
// Thread-Safe multiple Producer, multiple Consumer Queue
//
// http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
//

#ifndef CONCURRENT_QUEUE_HPP
#define CONCURRENT_QUEUE_HPP

#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <queue>

class NoTrigger{};

template<typename T, class Trigger = NoTrigger>
class concurrent_queue
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
	Trigger();
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
