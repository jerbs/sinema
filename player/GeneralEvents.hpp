#ifndef GENERAL_EVENTS_HPP
#define GENERAL_EVENTS_HPP

#include "Logging.hpp"

#include <boost/shared_ptr.hpp>
#include <time.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

class FileReader;
class Demuxer;
class VideoDecoder;
class AudioDecoder;
class VideoOutput;
class AudioOutput;
#ifdef SYNCTEST
class SyncTest;
#endif

struct InitEvent
{
    boost::shared_ptr<FileReader> fileReader;
    boost::shared_ptr<Demuxer> demuxer;
    boost::shared_ptr<VideoDecoder> videoDecoder;
    boost::shared_ptr<AudioDecoder> audioDecoder;
    boost::shared_ptr<VideoOutput> videoOutput;
    boost::shared_ptr<AudioOutput> audioOutput;
#ifdef SYNCTEST
    boost::shared_ptr<SyncTest> syncTest;
#endif
};

struct StartEvent
{
    StartEvent(){DEBUG();}
    ~StartEvent(){DEBUG();}
};

struct StopEvent
{
    StopEvent(){DEBUG();}
    ~StopEvent(){DEBUG();}
};

struct OpenFileEvent
{
    OpenFileEvent(std::string fn) : fileName(fn) {}
    std::string fileName;
};

struct CloseFileEvent
{
};

struct OpenAudioStreamReq
{
    OpenAudioStreamReq(int si, AVFormatContext* fc)
	: streamIndex(si),
	  avFormatContext(fc)
    {}
    int streamIndex;
    AVFormatContext* avFormatContext;
};

struct OpenAudioStreamResp
{
    OpenAudioStreamResp(int si) : streamIndex(si) {}
    int streamIndex;
};

struct OpenAudioStreamFail
{
    OpenAudioStreamFail(int si) : streamIndex(si) {}
    int streamIndex;
};

struct OpenVideoStreamReq
{
    OpenVideoStreamReq(int si, AVFormatContext* fc)
	: streamIndex(si),
	  avFormatContext(fc)
    {}
    int streamIndex;
    AVFormatContext* avFormatContext;
};

struct OpenVideoStreamResp
{
    OpenVideoStreamResp(int si) : streamIndex(si) {}
    int streamIndex;
};

struct OpenVideoStreamFail
{
    OpenVideoStreamFail(int si) : streamIndex(si) {}
    int streamIndex;
};

struct AudioPacketEvent
{
    AudioPacketEvent(AVPacket* avp)
	: avPacket(*avp)
    {
	av_dup_packet(&avPacket);
	DEBUG();
    }
    ~AudioPacketEvent()
    {
	DEBUG();
	av_free_packet(&avPacket);
    }
    AVPacket avPacket;
};

struct VideoPacketEvent
{
    VideoPacketEvent(AVPacket* avp)
	: avPacket(*avp)
    {
	av_dup_packet(&avPacket);
	DEBUG();
    }
    ~VideoPacketEvent()
    {
	DEBUG();
	av_free_packet(&avPacket);
    }
    AVPacket avPacket;
};

struct ConfirmPacketEvent
{
};

struct OpenAudioOutputReq
{
    unsigned int sample_rate;
    unsigned int channels;
    unsigned int frame_size;
};

struct OpenVideoOutputReq
{
    int width;
    int height;
};

struct ResizeVideoOutputReq
{
    int width;
    int height;
};

struct AudioSyncInfo
{
    AudioSyncInfo(double pts, 
		  struct timespec abstime)
	: pts(pts),
	  abstime(abstime)
    {}
    double pts;
    struct timespec abstime;
};

class XFVideoImage;

struct DeleteXFVideoImage
{
    DeleteXFVideoImage(boost::shared_ptr<XFVideoImage> image)
	: image(image)
    {}

    boost::shared_ptr<XFVideoImage> image;
};

#endif
