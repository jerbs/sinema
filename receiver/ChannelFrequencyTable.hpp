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
