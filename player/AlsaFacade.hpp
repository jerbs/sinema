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

#ifndef ALSA_FACADE_HPP
#define ALSA_FACADE_HPP

#include "player/GeneralEvents.hpp"
#include "player/AudioFrame.hpp"

extern "C"
{
#include <alsa/asoundlib.h>
}

#include <string>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

class CopyLog;

class AFPCMDigitalAudioInterface
{
public:
    typedef boost::function<void ()> send_audio_sync_info_fct_t;

    AFPCMDigitalAudioInterface(boost::shared_ptr<OpenAudioOutputReq> req);
    ~AFPCMDigitalAudioInterface();

    void setSendAudioSyncInfo(send_audio_sync_info_fct_t fct);

    bool play(boost::shared_ptr<AudioFrame> frame);
    bool getOverallLatency(snd_pcm_sframes_t& delay);
    snd_pcm_sframes_t getBufferFillLevel();
    double getNextPTS();

    void start();
    bool pause(bool enable);
    void stop();

    void skip(boost::shared_ptr<AudioFrame> frame, double seconds);

private:
    AFPCMDigitalAudioInterface();

    std::string device;
    snd_pcm_t* handle;
    snd_output_t* output;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;

    void setPcmHwParams();
    void setPcmSwParams();

    void dump();
    
    unsigned int sampleRate;
    unsigned int channels;
    unsigned int frameSize;
    snd_pcm_format_t format;
    unsigned int bytesPerSample;

    snd_pcm_uframes_t buffer_size;
    snd_pcm_uframes_t period_size;
    unsigned int buffer_time;        // ring buffer length in us
    unsigned int period_time;        // period time in us

    send_audio_sync_info_fct_t sendAudioSyncInfo;
    double nextPTS;  // PTS of the next frame written to playback buffer

    int xrun_recovery(int err);
    bool directWrite(boost::shared_ptr<AudioFrame> frame);
    bool copyFrame(const snd_pcm_channel_area_t *areas,
		   snd_pcm_uframes_t offset,
		   int frames, boost::shared_ptr<AudioFrame> frame);

    boost::shared_ptr<CopyLog> m_CopyLog;
};

#endif
