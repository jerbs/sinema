#ifndef GENERAL_EVENTS_HPP
#define GENERAL_EVENTS_HPP

#include <boost/shared_ptr.hpp>
#include <iostream>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#define DEBUG(x)
// #define DEBUG(x) std::cout << __PRETTY_FUNCTION__  x << std::endl
#define ERROR(x) std::cerr << "Error: " << __PRETTY_FUNCTION__  x << std::endl

class FileReader;
class Demuxer;
class VideoDecoder;
class AudioDecoder;
class VideoOutput;
class AudioOutput;

struct InitEvent
{
    boost::shared_ptr<FileReader> fileReader;
    boost::shared_ptr<Demuxer> demuxer;
    boost::shared_ptr<VideoDecoder> videoDecoder;
    boost::shared_ptr<AudioDecoder> audioDecoder;
    boost::shared_ptr<VideoOutput> videoOutput;
    boost::shared_ptr<AudioOutput> audioOutput;
};

struct QuitEvent
{
    QuitEvent(){DEBUG();}
    ~QuitEvent(){DEBUG();}
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
    AudioPacketEvent(AVPacket* avp) : avPacket(avp) {}
    AVPacket* avPacket;
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
    int sample_rate;
    int channels;
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

class XFVideoImage;

struct DeleteXFVideoImage
{
    DeleteXFVideoImage(boost::shared_ptr<XFVideoImage> image)
	: image(image)
    {}

    boost::shared_ptr<XFVideoImage> image;
};

#endif
