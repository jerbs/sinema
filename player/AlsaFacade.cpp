//
// ALSA Interface
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

#include "player/AlsaFacade.hpp"

#include <boost/thread/thread.hpp>
#include <iostream>
#include <stdio.h>

static char const * deviceName = "plug:default";
//static char const * deviceName = "plughw:Intel";
//static char const * deviceName = "plug:SLAVE=hw";
//static char const * deviceName = "plughw:0,0";

// ./amixer -D default scontents

// ===================================================================
// CopyLog copies the ALSA log into application log file:

class CopyLog
{
public:
    CopyLog();
    ~CopyLog();

    FILE* getWriteFilePointer();

private:
    void operator()();

    boost::thread copyLogThread;
    void copyLog();
    int& rfd;
    int& wfd;
    int filedes[2];
};

CopyLog::CopyLog()
    : rfd(filedes[0]),
      wfd(filedes[1])
{
    copyLogThread = boost::thread(boost::bind(&CopyLog::operator(), this));

    if (pipe(filedes) < 0)
    {
	TRACE_ERROR(<< "pipe failed: " << errno);
	exit(-1);
    }
}

CopyLog::~CopyLog()
{
    // Close write file descriptor to terminate the copy thread:
    close(wfd);
}

FILE* CopyLog::getWriteFilePointer()
{
    FILE* fd = fdopen(wfd, "w");
    setlinebuf(fd);
    return fd;
}

void CopyLog::operator()()
{
    TRACE_DEBUG(<< "tid = " << gettid());

    const size_t buffer_size = 1024;
    char buffer[buffer_size];
    while(1)
    {
	int ret = read(rfd, buffer, buffer_size-1);

	if (ret < 0)
	{
	    TRACE_ERROR(<< "read failed: " << ret);
	}
	else if (ret == 0)
	{
	    // End of file. Terminate thread:
	    return;
	}
	else
	{
      	    buffer[ret] = 0;
	    char* pos = buffer;

	    // Print each line with ALSA prefix:
	    while(*pos != 0)
	    {
		char* end = pos;
		while(1)
		{
		    if (*end == 0)
		    {
			break;
		    }
		    if (*end == '\n')
		    {
			*end = 0;
			end++;
			break;
		    }
		    end++;
		}

		TraceUnit traceUnit;
		traceUnit << "ALSA: " << pos;

		pos = end;
	    }
	}
    }
}

// ===================================================================

snd_pcm_format_t convert(SampleFormat sf)
{
    switch(sf)
    {
    case SAMPLE_FMT_NONE: return SND_PCM_FORMAT_UNKNOWN;
    case SAMPLE_FMT_U8:   return SND_PCM_FORMAT_U8;
    case SAMPLE_FMT_S16:  return SND_PCM_FORMAT_S16;
    case SAMPLE_FMT_S32:  return SND_PCM_FORMAT_S32;
    case SAMPLE_FMT_FLT:  return SND_PCM_FORMAT_FLOAT;
    case SAMPLE_FMT_DBL:  return SND_PCM_FORMAT_FLOAT64;
    default: break;
    }
    return SND_PCM_FORMAT_UNKNOWN;
}

// ===================================================================

AFPCMDigitalAudioInterface::AFPCMDigitalAudioInterface(boost::shared_ptr<OpenAudioOutputReq> req)
    : device(deviceName),
      handle(0),
      output(0),
      hwparams(0),
      swparams(0),
      sampleRate(req->sample_rate),
      channels(req->channels),
      frameSize(req->frame_size),
      format(convert(req->sample_format)),
      buffer_size(0),
      period_size(4096),
      buffer_time(500000), // ring buffer length in us
      period_time(100000), // period time in us
      nextPTS(0),
      m_CopyLog(new CopyLog())
{
    int ret;

    ret = snd_output_stdio_attach(&output, m_CopyLog->getWriteFilePointer(), 0);
    if (ret < 0)
    {
	TRACE_ERROR(<< "snd_output_stdio_attach failed: " << snd_strerror(ret));
	exit(-1);
    }

    ret = snd_pcm_hw_params_malloc(&hwparams);
    if (ret < 0)
    {
	TRACE_ERROR(<< "snd_pcm_hw_params_malloc failed: " << snd_strerror(ret));
	exit(-1);
    }

    ret = snd_pcm_sw_params_malloc(&swparams);
    if (ret < 0)
    {
	TRACE_ERROR(<< "snd_pcm_sw_params_malloc failed: " << snd_strerror(ret));
	exit(-1);
    }

    ret = snd_pcm_open(&handle, device.c_str(),
		       SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0)
    {
	TRACE_ERROR(<< "snd_pcm_open failed: " << snd_strerror(ret));
	exit(-1);
    }

    setPcmHwParams();
    setPcmSwParams();

    // bytes per sample
    bytesPerSample = snd_pcm_format_width(format) / 8;

    dump();
}

AFPCMDigitalAudioInterface::~AFPCMDigitalAudioInterface()
{
    snd_pcm_close(handle);
    snd_pcm_sw_params_free(swparams);
    snd_pcm_hw_params_free(hwparams);
}

void AFPCMDigitalAudioInterface::setSendAudioSyncInfo(send_audio_sync_info_fct_t fct)
{
    sendAudioSyncInfo = fct;
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
	TRACE_ERROR(<< "snd_pcm_hw_params_any failed: " << snd_strerror(ret));
	exit(-1);
    }

    TRACE_DEBUG(<< "Default hwparams:");
    snd_pcm_hw_params_dump(hwparams, output);

    ret = snd_pcm_hw_params_set_access(handle, hwparams,
				       SND_PCM_ACCESS_MMAP_INTERLEAVED);
    if (ret < 0)
    {
	TRACE_ERROR(<< "snd_pcm_hw_params_set_access failed: " << snd_strerror(ret));
	exit(-1);
    }

    ret = snd_pcm_hw_params_set_channels(handle, hwparams, channels);
    if (ret < 0)
    {
	TRACE_ERROR(<< "snd_pcm_hw_params_set_channels failed: ret=" << snd_strerror(ret));
	exit(-1);
    }

    ret = snd_pcm_hw_params_set_format(handle, hwparams, format);
    if (ret < 0)
    {
	TRACE_ERROR(<< "snd_pcm_hw_params_set_format failed: " << snd_strerror(ret));
	exit(-1);
    }

    const int enable = 1;
    // const int disable = 0;
    err = snd_pcm_hw_params_set_rate_resample(handle, hwparams, enable);
    if (err < 0)
    {
	TRACE_ERROR(<< "snd_pcm_hw_params_set_rate_resample failed: " << snd_strerror(err));
	exit(-1);;
    }

    unsigned int rrate = sampleRate;
    err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &rrate, 0);
    if (err < 0)
    {
	TRACE_ERROR(<< "Rate " << sampleRate << "Hz not available: " << snd_strerror(err));
	exit(-1);
    }

    if (rrate != sampleRate)
    {
	TRACE_ERROR( << "Rate doesn't match (requested " << sampleRate << "Hz, got " << rrate << "Hz)");
	exit(-1);
    }

    // Try to get a greater buffer:
    dir = 1;
    err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, &dir);
    if (err < 0)
    {
	TRACE_ERROR(<< "snd_pcm_hw_params_set_buffer_time_near failed: " << snd_strerror(err));
	exit(-1);
    }
    TRACE_DEBUG( << "buffer_time = " << buffer_time );

    err = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
    if (err < 0)
    {
	TRACE_ERROR(<< "snd_pcm_hw_params_get_buffer_size failed: " << snd_strerror(err));
	exit(-1);
    }
    TRACE_DEBUG( << "buffer_size = " << buffer_size );

    err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, &dir);
    if (err < 0)
    {
	TRACE_ERROR(<< "snd_pcm_hw_params_set_period_time_near failed: " << snd_strerror(err));
	exit(-1);
    }

    err = snd_pcm_hw_params_set_period_size_near(handle, hwparams, &period_size, &dir);
    if (err < 0)
    {
	TRACE_ERROR(<< "snd_pcm_hw_params_set_period_size_near failed: " << snd_strerror(err));
	exit(-1);
    }

    err = snd_pcm_hw_params_get_period_size(hwparams, &size, &dir);
    if (err < 0)
    {
	TRACE_ERROR(<< "snd_pcm_hw_params_get_period_size failed: " << snd_strerror(err));
	exit(-1);
    }
    period_size = size;

    TRACE_DEBUG(<< "Final HwParams:");
    snd_pcm_hw_params_dump(hwparams, output);

    ret = snd_pcm_hw_params(handle, hwparams);
    if (ret < 0)
    {
	TRACE_ERROR(<< "snd_pcm_hw_params failed: " << snd_strerror(ret));
	exit(-1);
    }
}

void AFPCMDigitalAudioInterface::setPcmSwParams()
{
    int err;

    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0)
    {
	TRACE_ERROR(<< "snd_pcm_sw_params_current failed: " << snd_strerror(err));
	exit(-1);
    }

    // Start transfer when the buffer is almost full:
    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (buffer_size / period_size) * period_size);
    if (err < 0)
    {
	TRACE_ERROR(<< "snd_pcm_sw_params_set_start_threshold failed: " << snd_strerror(err));
	exit(-1);
    }

#if 0
    // Allow the transfer when at least period_size samples can be processed:
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_size);
    if (err < 0)
    {
	TRACE_ERROR(<< "snd_pcm_sw_params_set_avail_min failed: " << snd_strerror(err));
	exit(-1);
    }
#endif

#if 0
    // Enable period events
    err = snd_pcm_sw_params_set_period_event(handle, swparams, 1);
    if (err < 0)
    {
	TRACE_ERROR(<< "snd_pcm_sw_params_set_period_event failed: " << snd_strerror(err));
	exit(-1);
    }
#endif

    TRACE_DEBUG(<< "Final SwParams:");
    snd_pcm_sw_params_dump(swparams, output);

    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0)
    {
	TRACE_ERROR(<< "snd_pcm_sw_params failed: " << snd_strerror(err));
	exit(-1);
    }
}

void AFPCMDigitalAudioInterface::dump()
{
    snd_pcm_hw_params_t* current_hwparams;

    int ret = snd_pcm_hw_params_malloc(&current_hwparams);
    if (ret < 0)
    {
	TRACE_ERROR(<< "snd_pcm_hw_params_malloc failed: " << snd_strerror(ret));
	exit(-1);
    }

    ret = snd_pcm_hw_params_current(handle, current_hwparams);
    if (ret < 0)
    {
	TRACE_ERROR(<< "snd_pcm_hw_params_current failed: " << snd_strerror(ret));
	exit(-1);
    }

    TRACE_DEBUG(<< "snd_pcm_hw_params_dump:");
    snd_pcm_hw_params_dump(current_hwparams, output);
    snd_pcm_hw_params_free(current_hwparams);

    TRACE_DEBUG(<< "snd_pcm_dump:");
    snd_pcm_dump(handle, output);
}

int AFPCMDigitalAudioInterface::xrun_recovery(int err)
{
    std::cout << "xrun_recovery err=" << snd_strerror(err) << std::endl;
    TRACE_DEBUG(<< "err=" << snd_strerror(err));

    if (err == -EPIPE)
    {
	// Under-run
	err = snd_pcm_prepare(handle);
	if (err < 0)
	{
	    TRACE_ERROR(<< "Can't recovery from underrun, prepare failed: " << snd_strerror(err));
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
		TRACE_ERROR(<< "Can't recovery from suspend, prepare failed: " << snd_strerror(err));
	}
	
	return 0;
    }

    return err;
}


bool AFPCMDigitalAudioInterface::play(boost::shared_ptr<AFAudioFrame> frame)
{
    bool finished;

    while(1)
    {
	snd_pcm_state_t state = snd_pcm_state(handle);
	TRACE_DEBUG(<< "state: " << snd_pcm_state_name(state));

	if (state == SND_PCM_STATE_XRUN)
	{
	    int err = xrun_recovery(-EPIPE);
	    if (err < 0)
	    {
		TRACE_ERROR(<< "XRUN recovery failed: " << snd_strerror(err));
		exit(EXIT_FAILURE);
	    }
	}
	else if (state == SND_PCM_STATE_SUSPENDED)
	{
	    int err = xrun_recovery(-ESTRPIPE);
	    if (err < 0)
	    {
		TRACE_ERROR(<< "SUSPEND recovery failed: " << snd_strerror(err));
		exit(EXIT_FAILURE);
	    }
	}
	else if (state == SND_PCM_STATE_SETUP)
	{
	    // This case occurs when restarting after snd_pcm_drop, not at
	    // the initial startup of playback.
	    int err = snd_pcm_prepare(handle);
	    if (err < 0)
	    {
		TRACE_ERROR(<< "snd_pcm_prepare failed: " << snd_strerror(err));
		exit(EXIT_FAILURE);
	    }
       	}

	// Get number of free samples in playback buffer. This call is
	// mandatory for updating the actual write pointer.
	snd_pcm_sframes_t avail = snd_pcm_avail_update(handle);

	if (avail < 0)
	{
	    TRACE_DEBUG(<< "snd_pcm_avail_update failed");
	    int err = xrun_recovery(avail);
	    if (err < 0)
	    {
		TRACE_ERROR(<< "avail update recovery failed: " << snd_strerror(err));
		exit(EXIT_FAILURE);
	    }
	    continue;
	}

	TRACE_DEBUG(<< "avail=" << avail << ", period_size=" << period_size);

	finished = directWrite(frame);

	if (state == SND_PCM_STATE_PREPARED)
	{
	    // Playback not yet started.

	    snd_pcm_uframes_t filled = buffer_size-avail;
	    if (2*filled > buffer_size)
	    {
		// Now starting playback should be possible
		// without getting a buffer underrun.
		start();

		// Now it is possible to determine the overall latency.
		// Send an AudioSyncInfo to VideoOutput as soon as possible:
		sendAudioSyncInfo();
	    }
	}

	break;
    }

    return finished;
}

void AFPCMDigitalAudioInterface::start()
{
    // This method is called by the play method when the buffer is half-filled.
    // If this condition would never become true, i.e. short file, this method
    // has to be called explicitly.

    snd_pcm_state_t state = snd_pcm_state(handle);
    if (state == SND_PCM_STATE_PREPARED)
    {
	TRACE_DEBUG(<< "snd_pcm_start");
	int err = snd_pcm_start(handle);
	if (err < 0)
	{
	    TRACE_ERROR(<< "snd_pcm_start failed: " << snd_strerror(err));
	    exit(EXIT_FAILURE);
	}
    }
}

void AFPCMDigitalAudioInterface::skip(boost::shared_ptr<AFAudioFrame> frame, double seconds)
{
    snd_pcm_uframes_t frames = seconds * double(sampleRate);
    int bytes = frames * channels * bytesPerSample;
    frame->consume(bytes);
}

bool AFPCMDigitalAudioInterface::directWrite(boost::shared_ptr<AFAudioFrame> frame)
{
    TRACE_DEBUG();

    const snd_pcm_channel_area_t *my_areas;
    snd_pcm_uframes_t offset;
    snd_pcm_sframes_t commitres;
    int err;

    // Number of frames(samples) available:
    snd_pcm_uframes_t frames = frame->getFrameByteSize()/ (channels * bytesPerSample);
    err = snd_pcm_mmap_begin(handle, &my_areas, &offset, &frames);
    // Here frames is the number of free continous samples in the playback buffer.
    if (err < 0)
    {
	TRACE_DEBUG(<< "snd_pcm_mmap_begin failed");
	if ((err = xrun_recovery(err)) < 0)
	{
	    TRACE_ERROR(<< "snd_pcm_mmap_begin recovery failed: " << snd_strerror(err));
	    exit(-1);
	}
    }

    TRACE_DEBUG(<<" frames=" << frames);

    if (frame->atBegin())
    {
	nextPTS = frame->getPTS();
    }
    bool finished = copyFrame(my_areas, offset, frames, frame);
    nextPTS += double(frames)/double(sampleRate);

    frame->setNextPTS(nextPTS);

    commitres = snd_pcm_mmap_commit(handle, offset, frames);
    if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames)
    {
	TRACE_ERROR(<< "snd_pcm_mmap_commit failed");
	if ((err = xrun_recovery(commitres >= 0 ? -EPIPE : commitres)) < 0)
	{
	    TRACE_ERROR( << "snd_pcm_mmap_commit recovery failed: " << snd_strerror(err));
	    exit(-1);
	}
    }

    return finished;
}

bool AFPCMDigitalAudioInterface::copyFrame(const snd_pcm_channel_area_t *areas,
					   snd_pcm_uframes_t offset,
					   int frames, boost::shared_ptr<AFAudioFrame> frame)
{
    char* src = frame->consume(frames * channels * bytesPerSample);

    unsigned char *samples[channels];
    int steps[channels]; 

    /* verify and prepare the contents of areas */
    for (unsigned int chn = 0; chn < channels; chn++)
    {
	if ((areas[chn].first % 8) != 0)
	{
	    TRACE_ERROR(<< "areas[" << chn << "].first == " << areas[chn].first << ", aborting...");
	    exit(EXIT_FAILURE);
	}

	samples[chn] = (((unsigned char *)areas[chn].addr) + (areas[chn].first / 8));

	if ((areas[chn].step % 8) != 0)
	{
	    TRACE_ERROR(<< "areas[" << chn << "].step == " << areas[chn].first << ", aborting...");
	    exit(EXIT_FAILURE);
	}

	steps[chn] = areas[chn].step / 8;
	samples[chn] += offset * steps[chn];
    }


    /* fill the channel areas */
    while (frames-- > 0)
    {
	for (unsigned int chn = 0; chn < channels; chn++)
	{
	    for (unsigned int byte = 0; byte < bytesPerSample; byte++)
		*(samples[chn] + byte) = *src++;

	    samples[chn] += steps[chn];
	}
    }

    return frame->atEnd();
}

bool AFPCMDigitalAudioInterface::getOverallLatency(snd_pcm_sframes_t& latency)
{
    snd_pcm_state_t state = snd_pcm_state(handle);

    if (state == SND_PCM_STATE_RUNNING)
    {
	int err = snd_pcm_delay(handle, &latency);

	if (err==0)
	{
	    TRACE_DEBUG(<< "latency=" << latency);
	    return true;
	}

	TRACE_ERROR(<< "snd_pcm_delay failed: " << snd_strerror(err));
	return false;
    }

    return false;
}

snd_pcm_sframes_t AFPCMDigitalAudioInterface::getBufferFillLevel()
{
    // Function returns the number of frames in the buffer.

    // Get number of free frames in the buffer:
    snd_pcm_sframes_t available = snd_pcm_avail(handle);
    if (available >= 0)
    {
	snd_pcm_sframes_t filled = buffer_size-available;

	TRACE_DEBUG( << "fillLevel=" << filled << "/" << buffer_size
		     << "=" << double(filled)/double(buffer_size)
		     << ", available=" << available );

	return filled;
    }
    else
    {
	TRACE_DEBUG( << "snd_pcm_avail failed: " << snd_strerror(available));
	return 0;
    }
}

double AFPCMDigitalAudioInterface::getNextPTS()
{
    return nextPTS;
}

bool AFPCMDigitalAudioInterface::pause(bool enable)
{
    if (snd_pcm_hw_params_can_pause(hwparams))
    {
	// Audio device supports pause.
	snd_pcm_pause(handle, enable ? 1 : 0);
	return true;
    }
    else if (enable)
    {
	// Pause is not supported by the audio device.
	// Immediately stop playback.
	snd_pcm_drop(handle);
    }
    return false;
}

void AFPCMDigitalAudioInterface::stop()
{
    // Stop PCM dropping pending frames, state SND_PCM_STATE_SETUP is entered:
    snd_pcm_drop(handle);
}
