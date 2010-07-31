//
// ALSA Mixer Interface
//
// Copyright (C) Joachim Erbs, 2009-2010
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

#ifndef ALSA_MIXER_HPP
#define ALSA_MIXER_HPP

#include "player/GeneralEvents.hpp"

extern "C"
{
#include <alsa/asoundlib.h>
}

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>

class AFMixerEventProcessor;

struct AlsaMixerElemEvent
{
    AlsaMixerElemEvent(snd_mixer_elem_t *elem)
	: elem(elem)
    {}
    snd_mixer_elem_t *elem;
};

class AFMixer
{
public:
    AFMixer(AudioOutput* audioOutput, MediaPlayer* mediaPlayer);
    ~AFMixer();

    void process(boost::shared_ptr<AlsaMixerElemEvent> event);

    void setPlaybackVolume(long volume);
    void setPlaybackSwitch(bool enabled);

private:

    void dump();
    void getVolumeAndSwitch(snd_mixer_elem_t* elem, long& volume, bool& enabled);
    bool isPlaybackVolumeElem(snd_mixer_elem_t* elem);
    void sendCurrentPlaybackValues();
    void setDefault(snd_mixer_elem_t* elem);
    void setVolume(snd_mixer_elem_t* elem, long volume);
    void setSwitch(snd_mixer_elem_t* elem, bool enabled);

    AudioOutput* audioOutput;
    MediaPlayer* mediaPlayer;
    std::string card;
    snd_mixer_t* handle;
    snd_mixer_elem_t* playbackVolumeElem;

    boost::thread eventProcessorThread;

    // Methods executed in the eventProcessorThread:
    void sendMixerElemEvent(snd_mixer_elem_t *elem);
    static int mixer_elem_event(snd_mixer_elem_t *elem,
				unsigned int mask);
    static int mixer_event(snd_mixer_t *mixer,
			   unsigned int mask,
			   snd_mixer_elem_t *elem);
};

#endif
