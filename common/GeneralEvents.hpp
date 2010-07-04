//
// Media Common Events
//
// Copyright (C) Joachim Erbs, 2010
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
    std::string name;
    std::string standard;
    std::string channel;
    int fine;
};

typedef std::vector<StationData> StationList;

struct ConfigurationData
{
    StationList stationList;
};

#ifdef CONFIG_FILE_TEST
#include "ConfigFileTest.hpp"
#endif

#endif
