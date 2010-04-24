//
// Play List
//
// Copyright (C) Joachim Erbs, 2009, 2010
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
    virtual bool erase();  // Remove the current play list entry and select the next one.
    virtual void clear();  // Remove all play list entries.

    std::string getCurrent();
    int getCurrentIndex();
    bool selectNext();
    bool selectPrevious();
    bool select(std::string file);
    int size();
    std::string operator[](int);


private:
    list m_list;
    iterator m_current;
};

#endif
