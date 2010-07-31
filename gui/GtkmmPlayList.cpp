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

#include "gui/GtkmmPlayList.hpp"
#include "platform/Logging.hpp"

PlayList::iterator GtkmmPlayList::append(std::string file)
{
    path p(1, size());
    iterator it = base::append(file);
    signal_entry_inserted(p, it);
    return it;
}

void GtkmmPlayList::clear()
{
    int n = size();
    while(n)
    {
	n--;
	path p(1, n);
	signal_entry_deleted(p);
    }
    base::clear();
}

bool GtkmmPlayList::erase()
{
    int erased_index = getCurrentIndex();
    if (base::erase())
    {
	path p(1, erased_index);
	signal_entry_deleted(p);
	return true;
    }
    return false;
}

bool GtkmmPlayList::erase(int n)
{
    if (base::erase(n))
    {
	path p(1, n);
	signal_entry_deleted(p);
	return true;
    }
    return false;
}

PlayList::iterator GtkmmPlayList::insert(int n, std::string elem)
{
    iterator it = base::insert(n, elem);
    if (it != end())
    {
	path p(1, n);
	signal_entry_inserted(p, it);
    }
    return it;
}

void GtkmmPlayList::on_entry_changed(int n, iterator it)
{
    TRACE_DEBUG(<< n);
    path p(1, n);
    signal_entry_changed(p, it);
}
