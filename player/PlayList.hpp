//
// Play List
//
// Copyright (C) Joachim Erbs, 2009-2010
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

#ifndef PLAY_LIST_HPP
#define PLAY_LIST_HPP

#include <list>
#include <string>
#include <vector>

class PlayList
{
public:
    typedef std::list<std::string> list;
    typedef list::iterator iterator;

    PlayList();
    virtual ~PlayList();

    virtual iterator append(std::string file);
    virtual void clear();  // Remove all play list entries.
    virtual bool erase();  // Remove the current play list entry and select the next one.
    virtual bool erase(int n);
    virtual iterator insert(int n, std::string);

    std::string getCurrent();
    int getCurrentIndex();
    int getIndex(iterator it);
    bool selectNext();
    bool selectPrevious();
    bool select(std::string file);
    void select(iterator);
    bool selectFirst();
    void selectEndOfList();
    int size();
    std::string operator[](int);

    iterator begin() {return m_list.begin();}
    iterator end() {return m_list.end();}
    iterator nth(int n);

protected:
    virtual void on_entry_changed(int n, iterator);

private:
    list m_list;
    iterator m_current;
};

#endif
