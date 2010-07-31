//
// Config File
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

#ifndef COMMON_CONFIG_FILE_HPP
#define COMMON_CONFIG_FILE_HPP

#include "common/GeneralEvents.hpp"
#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"

#include <string>

struct CommonInitEvent;
class MediaCommon;

class ConfigFile : public event_receiver<ConfigFile>
{
    friend class event_processor<>;

public:
    ConfigFile(event_processor_ptr_type evt_proc)
        : base_type(evt_proc)
    {}

    ~ConfigFile()
    {}

private:
    void process(boost::shared_ptr<CommonInitEvent>);
    void process(boost::shared_ptr<ConfigurationData>);

    void parse();
    void generate(ConfigurationData& configurationData);
    bool find();

    MediaCommon* mediaCommon;
    std::string fileName;
};

#endif
