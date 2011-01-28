//
// Media Common Events
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

#ifndef COMMON_GENERAL_EVENTS_HPP
#define COMMON_GENERAL_EVENTS_HPP

#ifdef CONFIG_FILE_TEST

// This always has to be included first to compile spirittest.

// For the spirittest application the class SpiritTest replaces MediaCommon:
#define MediaCommon ConfigFileTest

// Don't include the original header files for these classes:
#define COMMON_MEDIA_COMMON_HPP

#endif

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

class MediaCommon;
class ConfigFile;

struct CommonInitEvent
{
    MediaCommon* mediaCommon;
    boost::shared_ptr<ConfigFile> configFile;
};

struct ConfigurationFileWritten
{
};

struct StationData
{
    StationData()
	: fine(0)
    {}
    std::string name;
    std::string standard;
    std::string channel;
    int fine;
};

typedef std::vector<StationData> StationList;

struct ConfigurationGuiVisible
{
    ConfigurationGuiVisible(bool menuBar, bool toolBar, bool statusBar)
	: menuBar(menuBar),
	  toolBar(toolBar),
	  statusBar(statusBar)
    {}
    bool menuBar;
    bool toolBar;
    bool statusBar;
};

inline bool operator!=(const ConfigurationGuiVisible& lhs, const ConfigurationGuiVisible& rhs)
{
    return lhs.menuBar != rhs.menuBar
	|| lhs.toolBar != rhs.toolBar
	|| lhs.statusBar != rhs.statusBar;
}

struct ConfigurationGui
{
    ConfigurationGui()
	: visibleWindow(true, true, true),
	  visibleFullscreen(false, false, false)
    {}
    bool fullscreen;
    ConfigurationGuiVisible visibleWindow;
    ConfigurationGuiVisible visibleFullscreen;
};

struct ConfigurationPlayer
{
    ConfigurationPlayer()
	: useOptimalPixelFormat(true),
	  useXvClipping(true),
	  enableDeinterlacer(true)
    {}
    bool useOptimalPixelFormat;
    bool useXvClipping;
    bool enableDeinterlacer;
    std::string deinterlacer;
};

struct ConfigurationData
{
    StationList stationList;
    ConfigurationGui configGui;
    ConfigurationPlayer configPlayer;
};

#ifdef CONFIG_FILE_TEST
#include "ConfigFileTest.hpp"
#endif

#endif
