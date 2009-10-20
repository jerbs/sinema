//
// gtkmm Media Player
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef GTKMM_MEDIA_PLAYER_HPP
#define GTKMM_MEDIA_PLAYER_HPP

#include "player/MediaPlayer.hpp"

#include <glibmm/dispatcher.h>
#include <sigc++/signal.h>

class GtkmmMediaPlayer : public MediaPlayer
{
    static Glib::Dispatcher m_dispatcher;

public:
    sigc::signal<void, std::string> notificationFileName;
    sigc::signal<void, double> notificationDuration;
    sigc::signal<void, double> notificationCurrentTime;
    sigc::signal<void, NotificationCurrentVolume> notificationCurrentVolume;

    GtkmmMediaPlayer(boost::shared_ptr<PlayList> playList);

    static void notifyGuiThread();

    virtual void process(boost::shared_ptr<NotificationFileInfo> event);
    virtual void process(boost::shared_ptr<NotificationCurrentTime> event);
    virtual void process(boost::shared_ptr<NotificationCurrentVolume> event);
};

#endif
