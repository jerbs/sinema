//
// Media Common
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

#ifndef COMMON_MEDIA_COMMON_HPP
#define COMMON_MEDIA_COMMON_HPP

#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

struct ConfigurationData;
struct ConfigurationFileWritten;

class ConfigFile;

class MediaCommonThreadNotification
{
public:
    typedef void (*fct_t)();

    MediaCommonThreadNotification();
    static void setCallback(fct_t fct);

private:
    static fct_t m_fct;
};

class MediaCommon : public event_receiver<MediaCommon,
					  concurrent_queue<receive_fct_t, MediaCommonThreadNotification> >
{
    // The friend declaration allows to define the process methods private:
    friend class event_processor<concurrent_queue<receive_fct_t, MediaCommonThreadNotification> >;
    friend class MediaCommonThreadNotification;

public:
    MediaCommon();
    ~MediaCommon();

    void init();

    void saveConfigurationData(boost::shared_ptr<ConfigurationData> event);

    void processEventQueue();

protected:
    // EventReceiver
    boost::shared_ptr<ConfigFile> configFile;

private:
    // Boost threads:
    boost::thread commonThread;

    // EventProcessor:
    boost::shared_ptr<event_processor<> > commonEventProcessor;

    virtual void process(boost::shared_ptr<ConfigurationData> event) = 0;
    virtual void process(boost::shared_ptr<ConfigurationFileWritten>) {}

    void sendInitEvents();
};

#endif
