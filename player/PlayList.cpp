//
// Play List
//
// Copyright (C) Joachim Erbs, 2009, 2010
//

#include "player/PlayList.hpp"

#include <algorithm>

PlayList::PlayList()
    : m_current(m_list.end())
{
}

PlayList::~PlayList()
{
}

PlayList::iterator PlayList::append(std::string file)
{
    bool currentIsEndOfList = (m_current == m_list.end());
    m_list.push_back(file);
    iterator it = --m_list.end();
    if (currentIsEndOfList)
    {
	select(it);
	return it;
    }

    return it;
}

void PlayList::clear()
{
    m_list.clear();
    select(m_list.end());
}
bool PlayList::erase()
{
    if (m_current != m_list.end())
    {
	select(m_list.erase(m_current));
	return true;
    }

    return false;
}

bool PlayList::erase(int n)
{
    iterator it = nth(n);
    if (it == m_current)
    {
	select(m_list.end());
    }
    m_list.erase(it);
    return true;
}

PlayList::iterator PlayList::insert(int n, std::string elem)
{
    iterator it = nth(n);
    return m_list.insert(it, elem);
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
    return getIndex(m_current);
}

int PlayList::getIndex(iterator pos)
{
    int n = 0;
    iterator it = m_list.begin();
    while (it != m_list.end())
    {
	if (it == pos)
	{
	    return n;
	}
	it++;
	n++;
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
	select(tmp);
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
	iterator tmp = m_current;
	tmp--;
	select(tmp);
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
    select(it);
    return true;
}

void PlayList::select(iterator it)
{
    iterator old = m_current;
    m_current = it;
    on_entry_changed(getIndex(old), old);
    on_entry_changed(getIndex(it), it);
}


bool PlayList::selectFirst()
{
    select(m_list.begin());
    return (m_current != m_list.end());
}

void PlayList::selectEndOfList()
{
    select(m_list.end());
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

PlayList::iterator PlayList::nth(int n)
{
    iterator it = m_list.begin();
    while(n)
    {
	n--;
	it++;
    }
    return it;
}

void PlayList::on_entry_changed(int n, iterator)
{
}
