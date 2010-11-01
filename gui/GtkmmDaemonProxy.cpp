//
// Gtkmm Daemon Proxy
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

#include "gui/GtkmmDaemonProxy.hpp"
#include "receiver/TunerFacade.hpp"

Glib::Dispatcher GtkmmDaemonProxy::m_dispatcher;

GtkmmDaemonProxy::GtkmmDaemonProxy()
    : DaemonProxy()
{
    DaemonProxyThreadNotification::setCallback(&notifyGuiThread);
    m_dispatcher.connect(sigc::mem_fun(this, &DaemonProxy::processEventQueue));
}

GtkmmDaemonProxy::~GtkmmDaemonProxy()
{
}

void GtkmmDaemonProxy::notifyGuiThread()
{
    m_dispatcher();
}

void GtkmmDaemonProxy::process(boost::shared_ptr<TunerNotifyChannelTuned> event)
{
    notificationChannelTuned(event->channelData);
}

void GtkmmDaemonProxy::process(boost::shared_ptr<TunerNotifySignalDetected> event)
{
    notificationSignalDetected(event->channelData);
}

void GtkmmDaemonProxy::process(boost::shared_ptr<TunerScanStopped>)
{
    notificationScanStopped();
}

void GtkmmDaemonProxy::process(boost::shared_ptr<TunerScanFinished>)
{
    notificationScanFinished();
}
