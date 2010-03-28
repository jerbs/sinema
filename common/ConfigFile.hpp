//
// Config File
//
// Copyright (C) Joachim Erbs, 2010
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
