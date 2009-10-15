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
    sigc::signal<void, int> notificationDuration;
    sigc::signal<void, int> notificationCurrentTime;

    GtkmmMediaPlayer(boost::shared_ptr<PlayList> playList);

    static void notifyGuiThread();

    virtual void process(boost::shared_ptr<NotificationFileInfo> event);
    virtual void process(boost::shared_ptr<NotificationCurrentTime> event);
};

#endif
