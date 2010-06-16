#ifndef ALSA_FACADE_HPP
#define ALSA_FACADE_HPP

#include "GeneralEvents.hpp"

extern "C"
{
#include <alsa/asoundlib.h>
}

#include <string>
#include <boost/shared_ptr.hpp>

class AFAudioFrame
{
public:
    AFAudioFrame(int size)
	: allocatedByteSize(size),
	  frameByteSize(0),
	  buf(new char[size]),
	  pts(0),
	  offset(0)
    {}
    ~AFAudioFrame()
    {
	delete[](buf);
    }

    void setFrameByteSize(int size) {frameByteSize = size;}
    int getFrameByteSize() {return frameByteSize-offset;}
    int numAllocatedBytes() {return allocatedByteSize;}
    char* data() {return &buf[offset];}
    char* consume(int bytes)
    {
	int off = offset;
	offset += bytes;
	if (offset > frameByteSize)
	{
	    ERROR(<<"Consumed too much data.");
	    exit(-1);
	}
	return &buf[off];
    }

    void setPTS(double pts_) {pts = pts_;}
    double getPTS() {return pts;}

    bool finished()
    {
	if (offset == frameByteSize)
	{
	    offset = 0;
	    return true;
	}
	return false;
    }

private:
    AFAudioFrame();
    AFAudioFrame(const AFAudioFrame&);

    int allocatedByteSize;
    int frameByteSize;
    char* buf;
    double pts;
    int offset;
};

class AFPCMDigitalAudioInterface
{
public:
    AFPCMDigitalAudioInterface(boost::shared_ptr<OpenAudioOutputReq> req);
    ~AFPCMDigitalAudioInterface();

    bool play(boost::shared_ptr<AFAudioFrame> frame);
    snd_pcm_sframes_t getOverallLatency();

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

    snd_pcm_sframes_t buffer_size;
    snd_pcm_sframes_t period_size;
    unsigned int buffer_time;        // ring buffer length in us
    unsigned int period_time;        // period time in us
    
    int xrun_recovery(int err);

    bool first;

    bool directWrite(boost::shared_ptr<AFAudioFrame> frame);
    bool copyFrame(const snd_pcm_channel_area_t *areas,
		   snd_pcm_uframes_t offset,
		   int count, boost::shared_ptr<AFAudioFrame> frame);
};

#endif
