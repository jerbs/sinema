//
// Play List
//
// Copyright (C) Joachim Erbs, 2009
//

#include "player/PlayList.hpp"

void PlayList::append(std::string file)
{
    m_list.push_back(file);

    if (m_current == m_list.end())
    {
	// Added first element
	m_current = m_list.begin();
    }
}

void PlayList::erase()
{
    ListIter_t tmp = m_current;
    if (m_current != m_list.end())
    {
	m_current = m_list.erase(m_current);
    }
}

void PlayList::clear()
{
    m_list.clear();
    m_current = m_list.begin();
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

bool PlayList::selectNext()
{
    ListIter_t tmp = m_current;
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
