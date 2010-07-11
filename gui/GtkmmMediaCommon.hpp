//
// gtkmm Media Common
//
// Copyright (C) Joachim Erbs, 2010
//

#ifndef GTKMM_MEDIA_COMMON_HPP
#define GTKMM_MEDIA_COMMON_HPP

#include "common/MediaCommon.hpp"

#include <glibmm/dispatcher.h>
#include <sigc++/signal.h>

struct ConfigurationData;

class GtkmmMediaCommon : public MediaCommon
{
    static Glib::Dispatcher m_dispatcher;

public:
    sigc::signal<void, boost::shared_ptr<ConfigurationData> > signal_configuration_data_loaded;

    GtkmmMediaCommon();
    virtual ~GtkmmMediaCommon();

    static void notifyGuiThread();

private:
    virtual void process(boost::shared_ptr<ConfigurationData> event);
};

#endif
