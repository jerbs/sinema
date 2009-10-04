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

std::string PlayList::getPrevious()
{
    if (m_current != m_list.begin())
    {
	// Current element is not the first element
	m_current --;
	return getCurrent();
    }
    else
    {
	return std::string();
    }
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

std::string PlayList::getNext()
{
    if (m_current != m_list.end())
    {
	// Current element is not the last element
	m_current ++;
	return getCurrent();
    }
    else
    {
	return std::string();
    }
}
