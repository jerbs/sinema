//
// ALSA Mixer Interface
//
// Copyright (C) Joachim Erbs, 2009
//

#include "player/AlsaMixer.hpp"
#include "player/AudioOutput.hpp"
#include "player/MediaPlayer.hpp"

// -------------------------------------------------------------------

std::ostream& operator<<(std::ostream& strm, snd_mixer_elem_t* elem)
{
    snd_mixer_selem_id_t *sid;
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_get_id(elem, sid);

    strm << snd_mixer_selem_id_get_name(sid) << ", "
	 << snd_mixer_selem_id_get_index(sid);

    return strm;
}

// -------------------------------------------------------------------
// Callback class for boost::thread:

class AFMixerEventProcessor
{
public:
    AFMixerEventProcessor(snd_mixer_t* handle)
	: handle(handle)
    {}
    ~AFMixerEventProcessor()
    {}

    void operator()();

private:
    snd_mixer_t* handle;
};

// -------------------------------------------------------------------

AFMixer::AFMixer(AudioOutput* audioOutput, MediaPlayer* mediaPlayer)
    : audioOutput(audioOutput),
      mediaPlayer(mediaPlayer),
      card("default"),
      handle(0),
      playbackVolumeElem(0),
      pmin(0),
      pmax(0),
      pMono(false),
      pMuteSwitch(false)
{
    int ret;

    ret = snd_mixer_open(&handle, 0);
    if (ret < 0)
    {
	ERROR(<< "snd_mixer_open failed: " << snd_strerror(ret));
	exit(1);
    }

    ret = snd_mixer_attach(handle, card.c_str());
    if (ret < 0)
    {
	ERROR(<< "snd_mixer_attach failed: " << snd_strerror(ret));
	exit(1);
    }

    ret = snd_mixer_selem_register(handle, NULL, NULL);
    if (ret < 0)
    {
	ERROR(<< "snd_mixer_selem_register failed: " << snd_strerror(ret));
	exit(1);
    }

    snd_mixer_set_callback(handle, AFMixer::mixer_event);
    snd_mixer_set_callback_private(handle, (void*)this);

    ret = snd_mixer_load(handle);
    if (ret < 0)
    {
	ERROR(<< "snd_mixer_load failed: " << snd_strerror(ret));
	exit(1);
    }

    snd_mixer_elem_t* elem = snd_mixer_first_elem(handle);
    while (elem)
    {
	if (snd_mixer_selem_is_active(elem))
	{
	    DEBUG(<< "name = " << elem);

	    if (isPlaybackVolumeElem(elem))
	    {
		playbackVolumeElem = elem;
		sendCurrentPlaybackValues();
		break;
	    }
	}

	elem = snd_mixer_elem_next(elem);
    }

    // Start AFMixerEventProcessor instance in an own thread:
    AFMixerEventProcessor eventProcessor = AFMixerEventProcessor(handle);
    eventProcessorThread = boost::thread(eventProcessor);
}

AFMixer::~AFMixer()
{
    // Joining the thread currently blocks forever.
    eventProcessorThread.join();
    snd_mixer_close(handle);
}

void AFMixer::process(boost::shared_ptr<AlsaMixerElemEvent> event)
{
    if (event->elem == playbackVolumeElem)
    {
	sendCurrentPlaybackValues();
    }
}

void AFMixer::setPlaybackVolume(long volume)
{
    for (ChannelMap::iterator pos = channels.begin();
	 pos != channels.end();
	 pos++)
    {
	const snd_mixer_selem_channel_id_t& chn = pos->first;
	ChannelInfo& channelInfo = pos->second;
	snd_mixer_selem_set_playback_volume(playbackVolumeElem, chn, volume);
    }
}

void AFMixer::setPlaybackSwitch(bool enabled)
{
    for (ChannelMap::iterator pos = channels.begin();
	 pos != channels.end();
	 pos++)
    {
	const snd_mixer_selem_channel_id_t& chn = pos->first;
	ChannelInfo& channelInfo = pos->second;
	snd_mixer_selem_set_playback_switch(playbackVolumeElem, chn, enabled);
    }
}

bool AFMixer::isPlaybackVolumeElem(snd_mixer_elem_t* elem)
{
    bool hasPlaybackVolume = false;

    if (snd_mixer_selem_has_common_volume(elem))
    {
	hasPlaybackVolume = snd_mixer_selem_has_playback_volume_joined(elem);
    }
    else
    {
	hasPlaybackVolume = snd_mixer_selem_has_playback_volume(elem);
    }

    if (!hasPlaybackVolume)
    {
	return false;
    }

    if (snd_mixer_selem_has_common_switch(elem))
    {
	pMuteSwitch = snd_mixer_selem_has_playback_switch_joined(elem);
    }
    else
    {
	pMuteSwitch = snd_mixer_selem_has_playback_switch(elem);
    }

    // Remove all elements from container:
    channels.clear();

    if (snd_mixer_selem_is_playback_mono(elem))
    {
	pMono = true;
    }
    else
    {
	pMono = false;
	for (int i = 0; i <= SND_MIXER_SCHN_LAST; i++)
	{
	    snd_mixer_selem_channel_id_t chn = snd_mixer_selem_channel_id_t(i);
	    if (snd_mixer_selem_has_playback_channel(elem, chn))
	    {
		std::string name(snd_mixer_selem_channel_name(chn));
		channels.insert(ChannelMapValue(chn,ChannelInfo(name)));
	    }
	}
    }

    snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);

    return true;
}

void AFMixer::sendCurrentPlaybackValues()
{
    if (!playbackVolumeElem)
	return;

    long volume = 0;
    long count = 0;
    bool enabled = true;

    for (ChannelMap::iterator pos = channels.begin();
	 pos != channels.end();
	 pos++)
    {
	const snd_mixer_selem_channel_id_t& chn = pos->first;
	ChannelInfo& channelInfo = pos->second;
	
	long& pvol = channelInfo.volume;
	int psw;
	
	snd_mixer_selem_get_playback_volume(playbackVolumeElem, chn, &pvol);

	snd_mixer_selem_get_playback_switch(playbackVolumeElem, chn, &psw);

	channelInfo.enabled = psw;

	DEBUG(<< channelInfo.name << ": "
	      << channelInfo.volume << ", "
	      << (channelInfo.enabled ? "on" : "off"));

	volume += pvol;
	count++;
	enabled = enabled && psw;
    }

    boost::shared_ptr<NotificationCurrentVolume> event(new NotificationCurrentVolume());
    std::stringstream ss;
    ss << playbackVolumeElem;
    event->name = ss.str();
    event->volume = volume / count;
    event->enabled = enabled;
    event->minVolume = pmin;
    event->maxVolume = pmax;

    mediaPlayer->queue_event(event);
}

// -------------------------------------------------------------------
// AFMixer methods executed in eventProcessorThread:

void AFMixer::sendMixerElemEvent(snd_mixer_elem_t *elem)
{
    audioOutput->queue_event(boost::make_shared<AlsaMixerElemEvent>(elem));
}

int AFMixer::mixer_elem_event(snd_mixer_elem_t *elem,
			      unsigned int mask)
{
    AFMixer* obj = (AFMixer*)snd_mixer_elem_get_callback_private(elem);
    std::cout << "mixer_elem_event: " << std::hex << obj << std::endl;

    if (mask == SND_CTL_EVENT_MASK_REMOVE)
    {
	DEBUG(<< "mixer_elem_event: remove" << elem);
	return 0;
    }

    if (mask & SND_CTL_EVENT_MASK_INFO)
    {
	DEBUG(<< "mixer_elem_event: info" << elem)
    }

    if (mask & SND_CTL_EVENT_MASK_VALUE)
    {
	DEBUG(<< "mixer_elem_event: value" << elem);
	if (obj)
	    obj->sendMixerElemEvent(elem);
	else
	    ERROR(<< "mixer_elem_event: no obj!!!");
    }

    return 0;
}

int AFMixer::mixer_event(snd_mixer_t *mixer,
			 unsigned int mask,
			 snd_mixer_elem_t *elem)
{
    AFMixer* obj = (AFMixer*)snd_mixer_get_callback_private(mixer);
    DEBUG(<< "mixer_event: " << std::hex << obj);

    if (mask & SND_CTL_EVENT_MASK_ADD)
    {
	DEBUG(<< "mixer_event: add " << elem);
	snd_mixer_elem_set_callback(elem, AFMixer::mixer_elem_event);
	snd_mixer_elem_set_callback_private(elem, (void*)obj);
    }

    return 0;
}

// -------------------------------------------------------------------
// AFMixerEventProcessor main loop:

void AFMixerEventProcessor::operator()()
{
    // Currently it is not possible to terminate this function, i.e. to terminate the thread.
    // See ./alsamixer/mainloop.c for how to replace snd_mixer_wait 
    // with a poll based implementation using:
    // snd_mixer_poll_descriptors_count
    // snd_mixer_poll_descriptors
    // snd_mixer_poll_descriptors_revents

    while (1)
    {
	int ret = snd_mixer_wait(handle, -1);
	if (ret >= 0)
	{
	    ret = snd_mixer_handle_events(handle);
	    if (ret<0)
	    {
		ERROR(<< "snd_mixer_handle_events failed: " << snd_strerror(ret));
	    }
	}
	else
	{
	    ERROR(<< "snd_mixer_wait failed: ret=" << ret);
	}
    }
}

// -------------------------------------------------------------------
