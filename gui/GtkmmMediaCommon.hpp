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

#ifndef GTKMM_MEDIA_COMMON_HPP
#define GTKMM_MEDIA_COMMON_HPP

#include "common/MediaCommon.hpp"
#include "gui/GlibmmEventDispatcher.hpp"

#include <sigc++/signal.h>

struct ConfigurationData;

class GtkmmMediaCommon : public GlibmmEventDispatcher<MediaCommon>
{
public:
    sigc::signal<void, boost::shared_ptr<ConfigurationData> > signal_configuration_data_loaded;

    GtkmmMediaCommon();
    virtual ~GtkmmMediaCommon();

private:
    virtual void process(boost::shared_ptr<ConfigurationData> event);
};

#endif
