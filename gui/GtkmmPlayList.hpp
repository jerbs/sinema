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

    GtkmmPlayList() {}
    virtual ~GtkmmPlayList() {}

    virtual iterator append(std::string file);
    virtual void clear();  // Remove all play list entries.
    virtual bool erase();  // Remove the current play list entry and select the next one.
    virtual bool erase(int n);
    virtual iterator insert(int n, std::string);

protected:
    void on_entry_changed(int n, iterator);
};

#endif
