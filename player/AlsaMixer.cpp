//
// ALSA Mixer Interface
//
// Copyright (C) Joachim Erbs
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

static snd_mixer_selem_channel_id_t firstChannel() {return SND_MIXER_SCHN_FRONT_LEFT;}
static snd_mixer_selem_channel_id_t lastChannel() {return SND_MIXER_SCHN_LAST;}
static snd_mixer_selem_channel_id_t nextChannel(snd_mixer_selem_channel_id_t& chn)
{
    switch(chn)
    {
    case SND_MIXER_SCHN_UNKNOWN: chn = SND_MIXER_SCHN_FRONT_LEFT; break;
    case SND_MIXER_SCHN_FRONT_LEFT: chn = SND_MIXER_SCHN_FRONT_RIGHT; break;
    case SND_MIXER_SCHN_FRONT_RIGHT: chn = SND_MIXER_SCHN_REAR_LEFT; break;
    case SND_MIXER_SCHN_REAR_LEFT: chn = SND_MIXER_SCHN_REAR_RIGHT; break;
    case SND_MIXER_SCHN_REAR_RIGHT: chn = SND_MIXER_SCHN_FRONT_CENTER; break;
    case SND_MIXER_SCHN_FRONT_CENTER: chn = SND_MIXER_SCHN_WOOFER; break;
    case SND_MIXER_SCHN_WOOFER: chn = SND_MIXER_SCHN_SIDE_LEFT; break;
    case SND_MIXER_SCHN_SIDE_LEFT: chn = SND_MIXER_SCHN_SIDE_RIGHT; break;
    case SND_MIXER_SCHN_SIDE_RIGHT: chn = SND_MIXER_SCHN_REAR_CENTER; break;
    case SND_MIXER_SCHN_REAR_CENTER: chn = SND_MIXER_SCHN_LAST; break;
    case SND_MIXER_SCHN_LAST: chn = SND_MIXER_SCHN_LAST; break;
    }

    return chn;
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
      playbackVolumeElem(0)
{
    int ret;

    ret = snd_mixer_open(&handle, 0);
    if (ret < 0)
    {
	TRACE_ERROR(<< "snd_mixer_open failed: " << snd_strerror(ret));
	exit(1);
    }

    ret = snd_mixer_attach(handle, card.c_str());
    if (ret < 0)
    {
	TRACE_ERROR(<< "snd_mixer_attach failed: " << snd_strerror(ret));
	exit(1);
    }

    ret = snd_mixer_selem_register(handle, NULL, NULL);
    if (ret < 0)
    {
	TRACE_ERROR(<< "snd_mixer_selem_register failed: " << snd_strerror(ret));
	exit(1);
    }

    snd_mixer_set_callback(handle, AFMixer::mixer_event);
    snd_mixer_set_callback_private(handle, (void*)this);

    ret = snd_mixer_load(handle);
    if (ret < 0)
    {
	TRACE_ERROR(<< "snd_mixer_load failed: " << snd_strerror(ret));
	exit(1);
    }

    TRACE_DEBUG(<< "Searching playback volume element...");

    // For debugging only:
    dump();

    snd_mixer_elem_t* elem = snd_mixer_first_elem(handle);
    while (elem)
    {
	if (snd_mixer_selem_is_active(elem))
	{
	    TRACE_DEBUG(<< "name = " << elem);

	    if (isPlaybackVolumeElem(elem))
	    {
		if (!playbackVolumeElem)
		{
		    // Assuming that the first one is the important one.
		    TRACE_DEBUG(<< "Found playback volume element.");
		    playbackVolumeElem = elem;
		}
		setDefault(elem);
	    }
	}

	elem = snd_mixer_elem_next(elem);
    }

    sendCurrentPlaybackValues();

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
    setVolume(playbackVolumeElem, volume);
}

void AFMixer::setPlaybackSwitch(bool enabled)
{
    setSwitch(playbackVolumeElem, enabled);
}

void AFMixer::setVolume(snd_mixer_elem_t* elem,
			long volume)
{
    for (snd_mixer_selem_channel_id_t chn = firstChannel();
	 chn < lastChannel();
	 nextChannel(chn))
    {
	if (snd_mixer_selem_has_playback_channel(elem, chn))
	{
	    snd_mixer_selem_set_playback_volume(elem, chn, volume);
	}
    }
}

void AFMixer::setSwitch(snd_mixer_elem_t* elem,
			bool enabled)
{
    for (snd_mixer_selem_channel_id_t chn = firstChannel();
	 chn < lastChannel();
	 nextChannel(chn))
    {
	if (snd_mixer_selem_has_playback_channel(elem, chn))
	{
	    snd_mixer_selem_set_playback_switch(elem, chn, enabled);
	}
    }
}

void AFMixer::getVolumeAndSwitch(snd_mixer_elem_t* elem,
				 long& volume, bool& enabled)
{
    volume = 0;
    enabled = true;
    long count = 0;

    for (snd_mixer_selem_channel_id_t chn = firstChannel();
	 chn < lastChannel();
	 nextChannel(chn))
    {
	if (snd_mixer_selem_has_playback_channel(elem, chn))
	{	
	    long vol;
	    int swt;

	    snd_mixer_selem_get_playback_volume(elem, chn, &vol);
	    snd_mixer_selem_get_playback_switch(elem, chn, &swt);

	    TRACE_DEBUG(<< snd_mixer_selem_channel_name(chn) << ": "
			<< vol << ", "
			<< swt ? "on" : "off");

	    volume += vol;
	    count++;
	    enabled = enabled && swt;
	}
    }

    volume = count ? volume / count : 0;
}

bool AFMixer::isPlaybackVolumeElem(snd_mixer_elem_t* elem)
{
    if (snd_mixer_selem_has_common_volume(elem))
    {
	return snd_mixer_selem_has_playback_volume_joined(elem);
    }
    else
    {
	return snd_mixer_selem_has_playback_volume(elem);
    }
}

void AFMixer::sendCurrentPlaybackValues()
{
    if (!playbackVolumeElem)
	return;

    long volume;
    bool enabled;
    getVolumeAndSwitch(playbackVolumeElem, volume, enabled);

    long pmin;
    long pmax;
    snd_mixer_selem_get_playback_volume_range(playbackVolumeElem, &pmin, &pmax);

    boost::shared_ptr<NotificationCurrentVolume> event(new NotificationCurrentVolume());
    std::stringstream ss;
    ss << playbackVolumeElem;
    event->name = ss.str();
    event->volume = volume;
    event->enabled = enabled;
    event->minVolume = pmin;
    event->maxVolume = pmax;

    mediaPlayer->queue_event(event);
}

void AFMixer::setDefault(snd_mixer_elem_t* elem)
{
    snd_mixer_selem_id_t *sid;
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_get_id(elem, sid);

    const char* name = snd_mixer_selem_id_get_name(sid);

    long pmin;
    long pmax;
    snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);

    TRACE_DEBUG(<< name);

    if (strcmp(name, "Master") == 0 ||
	strcmp(name, "Front") == 0 ||
	strcmp(name, "Center") == 0)
    {
	TRACE_DEBUG(<< name << ", " << pmax);

	long volume;
	bool enabled;
	getVolumeAndSwitch(elem, volume, enabled);

	setSwitch(elem, true);
	if (volume < (pmax >> 2))
	    setVolume(elem, pmax >> 1);
    }
}

void AFMixer::dump()
{
    snd_mixer_elem_t* elem = snd_mixer_first_elem(handle);
    while (elem)
    {
	if (snd_mixer_selem_is_active(elem))
	{
	    TRACE_DEBUG(<< "name = " << elem);
	    TRACE_DEBUG(<< "  has_common_switch = " << snd_mixer_selem_has_common_switch(elem));
	    TRACE_DEBUG(<< "  has_playback_switch_joined = " << snd_mixer_selem_has_playback_switch_joined(elem));
	    TRACE_DEBUG(<< "  has_playback_switch = " << snd_mixer_selem_has_playback_switch(elem));
	    TRACE_DEBUG(<< "  has_common_volume = " << snd_mixer_selem_has_common_volume(elem));
	    TRACE_DEBUG(<< "  has_playback_volume_joined = " << snd_mixer_selem_has_playback_volume_joined(elem));
	    TRACE_DEBUG(<< "  has_playback_volume = " << snd_mixer_selem_has_playback_volume(elem));
	    TRACE_DEBUG(<< "  is_playback_mono = " << snd_mixer_selem_is_playback_mono(elem));

	    long pmin;
	    long pmax;
	    snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
	    TRACE_DEBUG(<< "  playback_volume_range: " << pmin << "..." << pmax);

	    for (snd_mixer_selem_channel_id_t chn = firstChannel();
		 chn < lastChannel();
		 nextChannel(chn))
	    {
		if (snd_mixer_selem_has_playback_channel(elem, chn))
		    TRACE_DEBUG(<< "    Channel " << chn
				<< ": " << snd_mixer_selem_channel_name(chn));
	    }
	}

	elem = snd_mixer_elem_next(elem);
    }
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
    TRACE_DEBUG( << "mixer_elem_event: " << std::hex << obj << std::dec);

    if (mask == SND_CTL_EVENT_MASK_REMOVE)
    {
	TRACE_DEBUG(<< "mixer_elem_event: remove" << elem);
	return 0;
    }

    if (mask & SND_CTL_EVENT_MASK_INFO)
    {
	TRACE_DEBUG(<< "mixer_elem_event: info" << elem)
    }

    if (mask & SND_CTL_EVENT_MASK_VALUE)
    {
	TRACE_DEBUG(<< "mixer_elem_event: value" << elem);
	if (obj)
	    obj->sendMixerElemEvent(elem);
	else
	    TRACE_ERROR(<< "mixer_elem_event: no obj!!!");
    }

    return 0;
}

int AFMixer::mixer_event(snd_mixer_t *mixer,
			 unsigned int mask,
			 snd_mixer_elem_t *elem)
{
    AFMixer* obj = (AFMixer*)snd_mixer_get_callback_private(mixer);
    TRACE_DEBUG(<< "mixer_event: " << std::hex << obj << std::dec);

    if (mask & SND_CTL_EVENT_MASK_ADD)
    {
	TRACE_DEBUG(<< "mixer_event: add " << elem);
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
		TRACE_ERROR(<< "snd_mixer_handle_events failed: " << snd_strerror(ret));
	    }
	}
	else
	{
	    TRACE_ERROR(<< "snd_mixer_wait failed: ret=" << ret);
	}
    }
}

// -------------------------------------------------------------------
