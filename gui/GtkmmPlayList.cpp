//
// Gtkmm Play List
//
// Copyright (C) Joachim Erbs, 2010
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
    DEBUG(<< n);
    path p(1, n);
    signal_entry_changed(p, it);
}
