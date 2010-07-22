//
// Play List
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef PLAY_LIST_HPP
#define PLAY_LIST_HPP

#include <list>
#include <string>

class PlayList
{
public:
    PlayList()
	: m_current(m_list.begin())
    {}
    ~PlayList() {}

    void append(std::string file);

    std::string getPrevious();
    std::string getCurrent();
    std::string getNext();

private:
    typedef std::list<std::string> List_t;
    typedef List_t::iterator ListIter_t;

    List_t m_list;
    ListIter_t m_current;
};

#endif