//
// Inter Thread Communication - Process Starter
//
// Copyright (C) Joachim Erbs, 2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef PROCESS_STARTER_HPP
#define PROCESS_STARTER_HPP

#include "platform/event_receiver.hpp"

#include <boost/foreach.hpp>

#include <string.h>
#include <sys/types.h>
#include <unistd.h>

template<typename Sender>
struct StartProcessRequest
{
    typedef StartProcessRequest<Sender> type;

    StartProcessRequest(boost::shared_ptr<Sender> sender,
			std::string command)
	: sender(sender),
	  command(command)
    {}

    type& operator()(std::string arg)
    {
	argv.clear();
	parameters.push_back(arg);
	return *this;
    }

    char const* const * getArgv()
    {
	if (argv.empty())
	{
	    argv.push_back(command.c_str());
	    // for (std::string& s : parameters)
	    BOOST_FOREACH(std::string& s, parameters)
	    {
		argv.push_back(s.c_str());
	    }
	    argv.push_back(0);
	}

	return &argv[0];
    }
    
    boost::shared_ptr<Sender> sender;
    const std::string command;
    std::vector<std::string> parameters;

private:
    std::vector<char const *> argv;
};

struct StartProcessResponse
{
};

struct StartProcessFailed
{
    StartProcessFailed(int error)
	: error(error)
    {}
    int error;
};

class process_starter : public event_receiver<process_starter>
{
    friend class event_processor<>;

public:
    process_starter(event_processor_ptr_type evt_proc)
        : base_type(evt_proc)
    {}
    ~process_starter()
    {}

private:
    template<typename Sender>
    void process(boost::shared_ptr<StartProcessRequest<Sender> > event)
    {
	TRACE_DEBUG(<< "command = " << event->command);

	int exec_errno = 0;

	// vfork(2) man page:
	//
	// CONFORMING TO
	// 4.3BSD, POSIX.1-2001. POSIX.1-2008 removes the specification of 
	// vfork(). The requirements put on vfork() by the standards are weaker 
	// than those put on fork(2), so an implementation where the two are
	// synonymous is compliant.  In particular, the programmer cannot rely 
	// on the parent remaining blocked until the child either terminates or 
	// calls execve(2), and cannot rely  on  any  specific  behavior  with
	// respect to shared memory.
	//
	// The Linux vfork currently blocks the parent and shares the memory.

	pid_t pid = vfork();

	if (pid == 0)
	{
	    // Child process.
	    // Here the parent is suspended until the child terminates or exec succeeds.
	    // Memory is still shared.

	    execvp(event->command.c_str(),
		   (char* const*)event->getArgv());

	    // execvp failed.
	    exec_errno = errno;

	    // Calling underscore exit, not exit, see 'man vfork'!
	    _exit(-1);
	}

	// Parent process.
	if (exec_errno)
	{
	    const size_t buflen = 256;
	    char buf[buflen];
	    char* errmsg = strerror_r(exec_errno, buf, buflen);

	    TRACE_ERROR(<< event->command.c_str()
			<< ", errno = " << exec_errno
			<< ", " << errmsg);

	    event->sender->queue_event(boost::make_shared<StartProcessFailed>(exec_errno));
	}
	else
	{
	    event->sender->queue_event(boost::make_shared<StartProcessResponse>());
	}
    }
};

#endif
