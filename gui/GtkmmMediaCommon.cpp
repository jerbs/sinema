//
// gtkmm Media Common
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

#include "gui/GtkmmMediaCommon.hpp"

Glib::Dispatcher GtkmmMediaCommon::m_dispatcher;

GtkmmMediaCommon::GtkmmMediaCommon()
    : MediaCommon()
{
    MediaCommonThreadNotification::setCallback(&notifyGuiThread);
    m_dispatcher.connect(sigc::mem_fun(this, &MediaCommon::processEventQueue));
}

GtkmmMediaCommon::~GtkmmMediaCommon()
{
}

void GtkmmMediaCommon::notifyGuiThread()
{
    m_dispatcher();
}

void GtkmmMediaCommon::process(boost::shared_ptr<ConfigurationData> event)
{
    signal_configuration_data_loaded(event);
}
