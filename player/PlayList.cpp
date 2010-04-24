//
// Play List
//
// Copyright (C) Joachim Erbs, 2009, 2010
//

#include "player/PlayList.hpp"

#include <algorithm>

PlayList::PlayList()
    : m_current(m_list.begin())
{
}

PlayList::~PlayList()
{
}

PlayList::iterator PlayList::append(std::string file)
{
    m_list.push_back(file);

    if (m_current == m_list.end())
    {
	// Added first element
	m_current = m_list.begin();
    }

    return --m_list.end();
}

bool PlayList::erase()
{
    if (m_current != m_list.end())
    {
	m_current = m_list.erase(m_current);
	return true;
    }

    return false;
}

void PlayList::clear()
{
    m_list.clear();
    m_current = m_list.end();
}

std::string PlayList::getCurrent()
{
    if (m_current != m_list.end())
    {
	return *m_current;
    }
    else
    {
	return std::string();
    }
}

int PlayList::getCurrentIndex()
{
    // If nothing is selected, then this function returns
    // an invalid index.
    int n = 0;
    iterator it = m_list.begin();
    while (it != m_list.end())
    {
	if (it == m_current)
	{
	    return n;
	}
	it++;
    }

    // Here n is the list size.
    return n;    
}

bool PlayList::selectNext()
{
    iterator tmp = m_current;
    // end() is the position after the last element
    if (++tmp != m_list.end())
    {
	// Current element is not the last element
	m_current = tmp;
	return true;
    }
    else
    {
	// No next element
	return false;
    }
}

bool PlayList::selectPrevious()
{
    // begin() is the first element
    if (m_current != m_list.begin())
    {
	// Current element is not the first element
	m_current --;
	return true;
    }
    else
    {
	// No previous element
	return false;
    }
}

bool PlayList::select(std::string file)
{
    iterator it = find(m_list.begin(), m_list.end(), file);
    if (it == m_list.end())
    {
	return false;
    }
    m_current = it;
    return true;
}

int PlayList::size()
{
    return m_list.size();
}

std::string PlayList::operator[](int n)
{
    iterator it = m_list.begin();
    while(n-->0) it++;
    return *it;
}
