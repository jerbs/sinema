//
// Gtkmm Play List
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
