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

#include <map>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

template<typename Sender>
struct StartProcessRequest
{
    friend class process_starter;
    typedef StartProcessRequest<Sender> type;

    StartProcessRequest(boost::shared_ptr<Sender> sender,
			std::string command)
	: sender(sender),
	  command(command)
    {}

    type& operator()(std::string arg)
    {
	parameters.push_back(arg);
	return *this;
    }

    void clearEnv()
    {
	env.clear();
    }

    void copyCurrentEnv()
    {
	char** entry = environ;
	while (*entry)
	{
	    // std::cout << *entry << std::endl;

	    char* begName = *entry;
	    char* endName = begName;
	    while(*endName != 0 && *endName != '=')
	    {
		endName++;
	    }

	    char* begValue = endName;
	    if (*begValue == '=') begValue++;
	    char* endValue = begValue;
	    while(*endValue != 0)
	    {
		endValue++;
	    }

	    std::string name(begName, endName);
	    std::string value(begValue, endValue);
	    
	    // std::cout << name << "==>" << value << std::endl;

	    env[name] = value;

	    entry++;
	}
    }

    void eraseEnv(std::string name)
    {
	env.erase(name);
    }

    void insertEnv(std::string name, std::string value)
    {
	env[name] = value;
    }
    
    boost::shared_ptr<Sender> sender;
    const std::string command;
    std::vector<std::string> parameters;
    std::map<std::string, std::string> env;

private:
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

    char const* const* getEnvp()
    {
	envp.clear();
	envn.clear();

	// typedef std::map<std::string, std::string>::iterator iterator;
	typedef std::pair<std::string, std::string> iterator;
	BOOST_FOREACH(iterator it, env)
	{
	    TRACE_DEBUG(<< it.first << "==>" << it.second);
	    std::string entry = it.first + "=" + it.second;
	    envn.push_back(entry);
	    envp.push_back(envn.back().c_str());
	}

	envp.push_back(0);
	return &envp[0];
    }

    std::vector<char const *> argv;
    std::vector<char const *> envp;
    std::list<std::string> envn;
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

	char const * const * argv = event->getArgv();
	char const * const * envp = event->getEnvp();

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

	    // execvpe is a GNU extension!
	    execvpe(event->command.c_str(),
		    (char* const*)argv,
		    (char* const*)envp);

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
