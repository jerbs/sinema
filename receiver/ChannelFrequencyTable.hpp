//
// Channel Frequency Table
//
// Copyright (C) Joachim Erbs, 2010
//

#ifndef CHANNEL_FREQUENCY_TABLE_HPP
#define CHANNEL_FREQUENCY_TABLE_HPP

#include <string.h>

class ChannelFrequencyTable
{
public:
    ChannelFrequencyTable();

    const char* getStandard_cstr() const;
    const char* getChannelName_cstr(int ch) const;
    int getChannelFrequency(int ch) const;
    bool operator()() const;
    bool isValid() const;

    static const ChannelFrequencyTable create(const char* standard);
    static const char* getStandard_cstr(int i);
    static const char* getChannelName_cstr(const ChannelFrequencyTable channelList, int ch);
    static int getChannelNumber(const ChannelFrequencyTable channelList, const char* channel);
    static int getChannelFreq(const ChannelFrequencyTable channelList, int ch);

private:
    ChannelFrequencyTable(int num);
    int num_standard;
};

#endif
