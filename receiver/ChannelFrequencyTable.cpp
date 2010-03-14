//
// Channel Frequency Table
//
// Copyright (C) Joachim Erbs, 2010
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
