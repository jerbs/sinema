//
// glibmm Event Dispatcher
//
// Copyright (C) Joachim Erbs, 2012
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

#ifndef GLIBMM_EVENT_DISPATCHER_HPP
#define GLIBMM_EVENT_DISPATCHER_HPP

#include <atomic>
#include <boost/bind.hpp>
#include <glibmm/dispatcher.h>

template<typename base_type>
class GlibmmEventDispatcher : public base_type
{
public:
    GlibmmEventDispatcher()
	: base_type(),
	  m_pendingProcessEventQueue(ATOMIC_FLAG_INIT)
    {
	initGlibmmEventDispatcher();
    }

    template<typename T>
    GlibmmEventDispatcher(T& p)
	: base_type(p),
	  m_pendingProcessEventQueue(ATOMIC_FLAG_INIT)
    {
	initGlibmmEventDispatcher();
    }

    template<typename T>
    GlibmmEventDispatcher(T const& p)
	: base_type(p),
	  m_pendingProcessEventQueue(ATOMIC_FLAG_INIT)
    {
	initGlibmmEventDispatcher();
    }

private:
    void initGlibmmEventDispatcher()
    {
	this->get_event_processor()->attach(boost::bind(&GlibmmEventDispatcher::notifyGuiThread, this));
	m_dispatcher.connect(sigc::mem_fun(this, &GlibmmEventDispatcher::processEventQueue));
    }

    void notifyGuiThread()
    {
	// This code may run in any thread, including the GUI thread.
	if (! m_pendingProcessEventQueue.test_and_set())
	{
	    m_dispatcher();
	}
    }

    void processEventQueue()
    {
	// This code is always running in the GUI thread.
	m_pendingProcessEventQueue.clear();
	this->get_event_processor()->dequeue_and_process_until_empty();
    }

private:
    Glib::Dispatcher m_dispatcher;
    std::atomic_flag m_pendingProcessEventQueue;
};

#endif
