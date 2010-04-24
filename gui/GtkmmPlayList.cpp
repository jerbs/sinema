//
// Gtkmm Play List
//
// Copyright (C) Joachim Erbs, 2010
//

#include "gui/GtkmmPlayList.hpp"

PlayList::iterator GtkmmPlayList::append(std::string file)
{
    path p(1, size());
    PlayList::iterator it = base::append(file);
    signal_entry_inserted(p, it);
    return it;
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
