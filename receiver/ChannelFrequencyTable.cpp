//
// Channel Frequency Table
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

#include "receiver/ChannelFrequencyTable.hpp"
#include "receiver/Frequencies.hpp"

ChannelFrequencyTable::ChannelFrequencyTable()
    : num_standard(-1)
{}

ChannelFrequencyTable::ChannelFrequencyTable(int num)
    : num_standard(num)
{}

const char* ChannelFrequencyTable::getStandard_cstr() const
{
    if (isValid())
	return chanlists[num_standard].name;
    else
	return 0;
}

const char* ChannelFrequencyTable::getChannelName_cstr(int ch) const
{
    if (isValid() &&
	0 <= ch &&
	ch < chanlists[num_standard].count)
	return chanlists[num_standard].list[ch].name;
    else
	return 0;
}

int ChannelFrequencyTable::getChannelFrequency(int ch) const
{
    if (isValid() &&
	0 <= ch &&
	ch < chanlists[num_standard].count)
	return chanlists[num_standard].list[ch].freq;
    else
	return 0;
}

bool ChannelFrequencyTable::operator()() const
{
    return isValid();
}

bool ChannelFrequencyTable::isValid() const
{
    return num_standard != -1;
}

// static methods:

const ChannelFrequencyTable ChannelFrequencyTable::create(const char* standard)
{
    int i = 0;
    while (chanlists[i].name != NULL)
    {
	if (strcmp(chanlists[i].name, standard) == 0)
	{
	    return ChannelFrequencyTable(i);
	}
	i++;
    }
    return ChannelFrequencyTable(-1);
}

const char* ChannelFrequencyTable::getStandard_cstr(int i)
{
    return chanlists[i].name;
}

const char* ChannelFrequencyTable::getChannelName_cstr(const ChannelFrequencyTable channelList, int ch)
{
    return channelList.getChannelName_cstr(ch);
}

int ChannelFrequencyTable::getChannelNumber(const ChannelFrequencyTable channelList, const char* channel)
{
    int ch = 0;
    while(const char* channelName = ChannelFrequencyTable::getChannelName_cstr(channelList, ch))
    {
	if (strcmp(channel, channelName) == 0)
	{
	    return ch;
	}
	ch++;
    }
    return -1;
}

int ChannelFrequencyTable::getChannelFreq(const ChannelFrequencyTable channelList, int ch)
{
    return channelList.getChannelFrequency(ch);
}
