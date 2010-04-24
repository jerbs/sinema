//
// Gtkmm Play List
//
// Copyright (C) Joachim Erbs, 2010
//

#ifndef GTKMM_PLAY_LIST_HPP
#define GTKMM_PLAY_LIST_HPP

#include "player/PlayList.hpp"

#include <sigc++/signal.h>

class GtkmmPlayList : public PlayList
{
public:
    typedef PlayList base;
    typedef std::vector<int> path;

    sigc::signal<void, const path&, const iterator&> signal_entry_changed;
    sigc::signal<void, const path&, const iterator&> signal_entry_inserted;
    sigc::signal<void, const path&> signal_entry_deleted;
    sigc::signal<void, const path&, const iterator&> signal_active_entry_changed;

    GtkmmPlayList() {}
    virtual ~GtkmmPlayList() {}

    virtual base::iterator append(std::string file);
    virtual bool erase();  // Remove the current play list entry and select the next one.
    virtual void clear();  // Remove all play list entries.
};

#endif
