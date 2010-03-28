//
// Media Common Events
//
// Copyright (C) Joachim Erbs, 2010
//

#ifndef COMMON_GENERAL_EVENTS_HPP
#define COMMON_GENERAL_EVENTS_HPP

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

#endif
