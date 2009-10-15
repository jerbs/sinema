//
// gtkmm Media Player
//
// Copyright (C) Joachim Erbs, 2009
//

#include "gui/GtkmmMediaPlayer.hpp"

Glib::Dispatcher GtkmmMediaPlayer::m_dispatcher;

GtkmmMediaPlayer::GtkmmMediaPlayer(boost::shared_ptr<PlayList> playList)
    : MediaPlayer(playList)
{
    MediaPlayerThreadNotification::setCallback(&notifyGuiThread);
    m_dispatcher.connect(sigc::mem_fun(this, &MediaPlayer::processEventQueue));	
}

void GtkmmMediaPlayer::notifyGuiThread()
{
    m_dispatcher();
}

void GtkmmMediaPlayer::process(boost::shared_ptr<NotificationFileInfo> event)
{
    notificationFileName(event->fileName);
    notificationDuration(event->duration);
    notificationCurrentTime(0);
}

void GtkmmMediaPlayer::process(boost::shared_ptr<NotificationCurrentTime> event)
{
    notificationCurrentTime(event->time);
}
