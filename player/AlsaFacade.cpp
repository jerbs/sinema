//
// ALSA Interface
//
// Copyright (C) Joachim Erbs, 2009
//

#include "player/AlsaFacade.hpp"

#include <iostream>

static char const * deviceName = "plug:default";
//static char const * deviceName = "plughw:Intel";
//static char const * deviceName = "plug:SLAVE=hw";
//static char const * deviceName = "plughw:0,0";

AFPCMDigitalAudioInterface::AFPCMDigitalAudioInterface(boost::shared_ptr<OpenAudioOutputReq> req)
    : device(deviceName),
      handle(0),
      output(0),
      hwparams(0),
      swparams(0),
      sampleRate(req->sample_rate),
      channels(req->channels),
      frameSize(req->frame_size),
      format(SND_PCM_FORMAT_S16),  // sample format
      buffer_size(0),
      period_size(4096),
      buffer_time(500000), // ring buffer length in us
      period_time(100000), // period time in us
      nextPTS(0),
      first(true)
{
    int ret;

    ret = snd_output_stdio_attach(&output, stdout, 0);
    if (ret < 0)
    {
	ERROR(<< "snd_output_stdio_attach failed: " << snd_strerror(ret));
	exit(-1);
    }

    ret = snd_pcm_hw_params_malloc(&hwparams);
    if (ret < 0)
    {
	ERROR(<< "snd_pcm_hw_params_malloc failed: " << snd_strerror(ret));
	exit(-1);
    }

    ret = snd_pcm_sw_params_malloc(&swparams);
    if (ret < 0)
    {
	ERROR(<< "snd_pcm_sw_params_malloc failed: " << snd_strerror(ret));
	exit(-1);
    }

    ret = snd_pcm_open(&handle, device.c_str(),
		       SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0)
    {
	ERROR(<< "snd_pcm_open failed: " << snd_strerror(ret));
	exit(-1);
    }

    setPcmHwParams();
    setPcmSwParams();

    // bytes per sample
    bytesPerSample = snd_pcm_format_width(format) / 8;
    if (bytesPerSample != 2)
    {
	ERROR(<< "Expecting format with 2 bytes per sample per channel.");
	exit(-1);
    }

    dump();
}

AFPCMDigitalAudioInterface::~AFPCMDigitalAudioInterface()
{
    snd_pcm_close(handle);
    snd_pcm_sw_params_free(swparams);
    snd_pcm_hw_params_free(hwparams);
}

void AFPCMDigitalAudioInterface::setPcmHwParams()
{
    int ret;
    int err;
    snd_pcm_uframes_t size;
    int dir;
 
    ret = snd_pcm_hw_params_any(handle, hwparams);
    if (ret < 0)
    {
	ERROR(<< "snd_pcm_hw_params_any failed: " << snd_strerror(ret));
	exit(-1);
    }

    DEBUG(<< "Default hwparams:");
    snd_pcm_hw_params_dump(hwparams, output);

    ret = snd_pcm_hw_params_set_access(handle, hwparams,
				       SND_PCM_ACCESS_MMAP_INTERLEAVED);
    if (ret < 0)
    {
	ERROR(<< "snd_pcm_hw_params_set_access failed: " << snd_strerror(ret));
	exit(-1);
    }

    ret = snd_pcm_hw_params_set_channels(handle, hwparams, channels);
    if (ret < 0)
    {
	ERROR(<< "snd_pcm_hw_params_set_channels failed: ret=" << snd_strerror(ret));
	exit(-1);
    }

    ret = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16);
    if (ret < 0)
    {
	ERROR(<< "snd_pcm_hw_params_set_format failed: " << snd_strerror(ret));
	exit(-1);
    }

    const int enable = 1;
    // const int disable = 0;
    err = snd_pcm_hw_params_set_rate_resample(handle, hwparams, enable);
    if (err < 0)
    {
	ERROR(<< "snd_pcm_hw_params_set_rate_resample failed: " << snd_strerror(err));
	exit(-1);;
    }

    unsigned int rrate = sampleRate;
    err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &rrate, 0);
    if (err < 0)
    {
	ERROR(<< "Rate " << sampleRate << "Hz not available: " << snd_strerror(err));
	exit(-1);
    }

    if (rrate != sampleRate)
    {
	ERROR( << "Rate doesn't match (requested " << sampleRate << "Hz, got " << rrate << "Hz)");
	exit(-1);
    }

    // Try to get a greater buffer:
    dir = 1;
    err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, &dir);
    if (err < 0)
    {
	ERROR(<< "snd_pcm_hw_params_set_buffer_time_near failed: " << snd_strerror(err));
	exit(-1);
    }
    DEBUG( << "buffer_time = " << buffer_time );

    err = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
    if (err < 0)
    {
	ERROR(<< "snd_pcm_hw_params_get_buffer_size failed: " << snd_strerror(err));
	exit(-1);
    }
    DEBUG( << "buffer_size = " << buffer_size );

    err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, &dir);
    if (err < 0)
    {
	ERROR(<< "snd_pcm_hw_params_set_period_time_near failed: " << snd_strerror(err));
	exit(-1);
    }

    err = snd_pcm_hw_params_set_period_size_near(handle, hwparams, &period_size, &dir);
    if (err < 0)
    {
	ERROR(<< "snd_pcm_hw_params_set_period_size_near failed: " << snd_strerror(err));
	exit(-1);
    }

    err = snd_pcm_hw_params_get_period_size(hwparams, &size, &dir);
    if (err < 0)
    {
	ERROR(<< "snd_pcm_hw_params_get_period_size failed: " << snd_strerror(err));
	exit(-1);
    }
    period_size = size;

    DEBUG(<< "Final HwParams:");
    snd_pcm_hw_params_dump(hwparams, output);

    ret = snd_pcm_hw_params(handle, hwparams);
    if (ret < 0)
    {
	ERROR(<< "snd_pcm_hw_params failed: " << snd_strerror(ret));
	exit(-1);
    }
}

void AFPCMDigitalAudioInterface::setPcmSwParams()
{
    int err;

    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0)
    {
	ERROR(<< "snd_pcm_sw_params_current failed: " << snd_strerror(err));
	exit(-1);
    }

    // Start transfer when the buffer is almost full:
    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (buffer_size / period_size) * period_size);
    if (err < 0)
    {
	ERROR(<< "snd_pcm_sw_params_set_start_threshold failed: " << snd_strerror(err));
	exit(-1);
    }

#if 0
    // Allow the transfer when at least period_size samples can be processed:
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_size);
    if (err < 0)
    {
	ERROR(<< "snd_pcm_sw_params_set_avail_min failed: " << snd_strerror(err));
	exit(-1);
    }
#endif

#if 0
    // Enable period events
    err = snd_pcm_sw_params_set_period_event(handle, swparams, 1);
    if (err < 0)
    {
	ERROR(<< "snd_pcm_sw_params_set_period_event failed: " << snd_strerror(err));
	exit(-1);
    }
#endif

    DEBUG(<< "Final SwParams:");
    snd_pcm_sw_params_dump(swparams, output);

    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0)
    {
	ERROR(<< "snd_pcm_sw_params failed: " << snd_strerror(err));
	exit(-1);
    }
}

void AFPCMDigitalAudioInterface::dump()
{
    DEBUG(<< "snd_pcm_dump:");
    snd_pcm_dump(handle, output);
}

int AFPCMDigitalAudioInterface::xrun_recovery(int err)
{
    std::cout << "xrun_recovery" << std::endl;
    DEBUG(<< "err=" << snd_strerror(err));

    if (err == -EPIPE)
    {
	// Under-run
	err = snd_pcm_prepare(handle);
	if (err < 0)
	{
	    ERROR(<< "Can't recovery from underrun, prepare failed: " << snd_strerror(err));
	    return 0;
	}
    }
    else if (err == -ESTRPIPE)
    {
	// Wait until the suspend flag is released
	while ((err = snd_pcm_resume(handle)) == -EAGAIN)
	{
	    sleep(1);
	}
	
	if (err < 0)
	{
	    err = snd_pcm_prepare(handle);
	    if (err < 0)
		ERROR(<< "Can't recovery from suspend, prepare failed: " << snd_strerror(err));
	}
	
	return 0;
    }

    return err;
}


bool AFPCMDigitalAudioInterface::play(boost::shared_ptr<AFAudioFrame> frame)
{
    DEBUG();

    bool finished;

    while(1)
    {
	snd_pcm_state_t state = snd_pcm_state(handle);
	if (state == SND_PCM_STATE_XRUN)
	{
	    int err = xrun_recovery(-EPIPE);
	    if (err < 0)
	    {
		printf("XRUN recovery failed: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	    }
	    first = true;
	}
	else if (state == SND_PCM_STATE_SUSPENDED)
	{
	    int err = xrun_recovery(-ESTRPIPE);
	    if (err < 0)
	    {
		printf("SUSPEND recovery failed: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	    }
	}

	snd_pcm_sframes_t avail = snd_pcm_avail_update(handle);

	if (avail < 0)
	{
	    int err = xrun_recovery(avail);
	    if (err < 0)
	    {
		printf("avail update recovery failed: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	    }
	    first = true;
	    continue;
	}

	DEBUG(<< "avail=" << avail << ", period_size=" << period_size);

	finished = directWrite(frame);

	// if (0 && avail > 0)
	{
	    if (first)
	    {
		first = false;
		DEBUG(<< "snd_pcm_start");
		int err = snd_pcm_start(handle);
		if (err < 0)
		{
		    ERROR(<< "snd_pcm_start failed: " << snd_strerror(err));
		    exit(EXIT_FAILURE);
		}
	    }
	    else
	    {
		break;
	    }
	    continue;
	}
	break;
    }

    return finished;
}

bool AFPCMDigitalAudioInterface::directWrite(boost::shared_ptr<AFAudioFrame> frame)
{
    DEBUG();

    const snd_pcm_channel_area_t *my_areas;
    snd_pcm_uframes_t offset;
    snd_pcm_sframes_t commitres;
    int err;

    // Number of frames(samples) available:
    snd_pcm_uframes_t frames = frame->getFrameByteSize()/ (channels * 2);
    err = snd_pcm_mmap_begin(handle, &my_areas, &offset, &frames);
    // Here frames is the number of free continous samples in the playback buffer.
    if (err < 0)
    {
	if ((err = xrun_recovery(err)) < 0)
	{
	    ERROR(<< "snd_pcm_mmap_begin recovery failed: " << snd_strerror(err));
	    exit(-1);
	}
	first = 1;
    }

    DEBUG(<<" frames=" << frames);
    if (frame->atBegin())
    {
	nextPTS = frame->getPTS();
    }
    bool finished = copyFrame(my_areas, offset, frames, frame);
    nextPTS += double(frames)/double(sampleRate);

    commitres = snd_pcm_mmap_commit(handle, offset, frames);
    if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames)
    {
	if ((err = xrun_recovery(commitres >= 0 ? -EPIPE : commitres)) < 0)
	{
	    ERROR( << "snd_pcm_mmap_commit recovery failed: " << snd_strerror(err));
	    exit(-1);
	}
	first = 1;
    }

    return finished;
}

bool AFPCMDigitalAudioInterface::copyFrame(const snd_pcm_channel_area_t *areas,
					   snd_pcm_uframes_t offset,
					   int frames, boost::shared_ptr<AFAudioFrame> frame)
{
    u_int16_t* src = (u_int16_t*)frame->consume(frames*2*channels);

    unsigned char *samples[channels];
    int steps[channels]; 

    /* verify and prepare the contents of areas */
    for (unsigned int chn = 0; chn < channels; chn++)
    {
	if ((areas[chn].first % 8) != 0)
	{
	    printf("areas[%i].first == %i, aborting...\n", chn, areas[chn].first);
	    exit(EXIT_FAILURE);
	}

	samples[chn] = (((unsigned char *)areas[chn].addr) + (areas[chn].first / 8));

	if ((areas[chn].step % 16) != 0)
	{
	    printf("areas[%i].step == %i, aborting...\n", chn, areas[chn].step);
	    exit(EXIT_FAILURE);
	}

	steps[chn] = areas[chn].step / 8;
	samples[chn] += offset * steps[chn];
    }


    /* fill the channel areas */
    while (frames-- > 0)
    {
	union
	{
	    int i;
	    unsigned char c[4];
	} ires;

	for (unsigned int chn = 0; chn < channels; chn++)
	{
	    ires.i = *src++;

	    for (unsigned int byte = 0; byte < bytesPerSample; byte++)
		*(samples[chn] + byte) = ires.c[byte]++;

	    samples[chn] += steps[chn];
	}
    }

    return frame->atEnd();
}

bool AFPCMDigitalAudioInterface::getOverallLatency(snd_pcm_sframes_t& latency)
{
    DEBUG();

    snd_pcm_state_t state = snd_pcm_state(handle);

    if (state == SND_PCM_STATE_RUNNING)
    {
	int err = snd_pcm_delay(handle, &latency);

	if (err==0)
	{
	    DEBUG(<< "latency=" << latency);
	    return true;
	}

	ERROR(<< "snd_pcm_delay failed: " << snd_strerror(err));
	return false;
    }

    ERROR(<< "Playback not running: " << snd_pcm_state_name(state));
    return false;
}

snd_pcm_sframes_t AFPCMDigitalAudioInterface::getBufferFillLevel()
{
    // Get number of free frames in the buffer:
    snd_pcm_sframes_t available = snd_pcm_avail(handle);
    snd_pcm_sframes_t filled = buffer_size-available;

    DEBUG( << "fillLevel=" << filled << "/" << buffer_size
	   << "=" << double(filled)/double(buffer_size) );

    return filled;
}

double AFPCMDigitalAudioInterface::getNextPTS()
{
    return nextPTS;
}

void AFPCMDigitalAudioInterface::pause(bool enable)
{
    // Pause is not supported by all hardware: snd_pcm_hw_params_can_pause()
    snd_pcm_pause(handle, enable ? 1 : 0);
}

void AFPCMDigitalAudioInterface::stop()
{
    // Stop PCM dropping pending frames:
    snd_pcm_drop(handle);
}
