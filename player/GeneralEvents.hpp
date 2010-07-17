//
// Media Player Events
//
// Copyright (C) Joachim Erbs, 2009, 2010
//

#ifndef PLAYER_GENERAL_EVENTS_HPP
#define PLAYER_GENERAL_EVENTS_HPP


#ifdef SYNCTEST

// This always has to be included first to compile synctest.

// For the synctest application the class SyncTest replaces
// Demuxer, MediaPlayer, AudioDecoder and VideoDecoder:
#define MediaPlayer SyncTest
#define Demuxer SyncTest
#define AudioDecoder SyncTest
#define VideoDecoder SyncTest

#define MediaPlayerThreadNotification NoTrigger

// Don't include the original header files for these classes:
#define MEDIA_PLAYER_HPP
#define DEMUXER_HPP
#define AUDIO_DECODER_HPP
#define VIDEO_DECODER_HPP

#endif


#include "platform/Logging.hpp"

#include <boost/shared_ptr.hpp>
#include <list>
#include <string>
#include <time.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <string>

class MediaPlayer;
class Demuxer;
class VideoDecoder;
class AudioDecoder;
class VideoOutput;
class AudioOutput;
class Deinterlacer;

// ===================================================================
// General Events

struct InitEvent
{
    MediaPlayer* mediaPlayer;
    boost::shared_ptr<Demuxer> demuxer;
    boost::shared_ptr<VideoDecoder> videoDecoder;
    boost::shared_ptr<AudioDecoder> audioDecoder;
    boost::shared_ptr<VideoOutput> videoOutput;
    boost::shared_ptr<AudioOutput> audioOutput;
    boost::shared_ptr<Deinterlacer> deinterlacer;
};

struct StartEvent
{
    StartEvent(){TRACE_DEBUG();}
    ~StartEvent(){TRACE_DEBUG();}
};

struct StopEvent
{
    StopEvent(){TRACE_DEBUG();}
    ~StopEvent(){TRACE_DEBUG();}
};

// ===================================================================
// 

struct OpenFileReq
{
    OpenFileReq(std::string fn) : fileName(fn) {}
    std::string fileName;
};

struct OpenFileResp {};
struct OpenFileFail
{
    enum Reason {
	OpenFileFailed,
	FindStreamFailed,
	OpenStreamFailed,
	AlreadyOpened
    };

    OpenFileFail(Reason reason) : reason(reason) {}
    Reason reason;
};

struct CloseFileReq {};
struct CloseFileResp {};

struct OpenAudioStreamFailed {};
struct OpenVideoStreamFailed {};

std::ostream& operator<<(std::ostream& strm, OpenFileFail::Reason reason);

// ===================================================================

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

// ===================================================================

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

// ===================================================================

struct CloseAudioStreamReq {};
struct CloseAudioStreamResp {};
struct CloseVideoStreamReq {};
struct CloseVideoStreamResp {};

// ===================================================================

struct SeekAbsoluteReq
{
    SeekAbsoluteReq(int64_t seekTarget)
	: seekTarget(seekTarget)
    {}
    int64_t seekTarget;  // seconds * AV_TIME_BASE
};

struct SeekRelativeReq
{
    SeekRelativeReq(int64_t seekOffset)
	: seekOffset(seekOffset),
	  displayedFramePTS(0)
    {}
    int64_t seekOffset;  // seconds * AV_TIME_BASE
    double displayedFramePTS;
};

struct FlushReq {};

struct AudioFlushedInd {};

// ===================================================================

struct AudioPacketEvent
{
    AudioPacketEvent(AVPacket* avp)
	: avPacket(*avp)
    {
	av_dup_packet(&avPacket);
    }
    ~AudioPacketEvent()
    {
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
    }
    ~VideoPacketEvent()
    {
	av_free_packet(&avPacket);
    }
    AVPacket avPacket;
};

struct ConfirmAudioPacketEvent
{
};

struct ConfirmVideoPacketEvent
{
};

// ===================================================================

struct OpenAudioOutputReq
{
    unsigned int sample_rate;
    unsigned int channels;
    unsigned int frame_size;
};

struct OpenAudioOutputResp{};
struct OpenAudioOutputFail{};

struct CloseAudioOutputReq{};
struct CloseAudioOutputResp{};

// ===================================================================

struct OpenVideoOutputReq
{
    OpenVideoOutputReq(int width, int height,
		       int parNum, int parDen,
		       int imageFormat)
	: width(width),
	  height(height),
	  parNum(parNum),
	  parDen(parDen),
	  imageFormat(imageFormat)
    {}
    int width;
    int height;
    int parNum;  // pixel aspect ratio numerator
    int parDen;  // pixel aspect ratio denominator
    int imageFormat;
};

struct OpenVideoOutputResp{};
struct OpenVideoOutputFail{};

struct CloseVideoOutputReq{};
struct CloseVideoOutputResp{};

struct ResizeVideoOutputReq
{
    ResizeVideoOutputReq(int width, int height,
			 int parNum, int parDen,
			 int imageFormat)
	: width(width),
	  height(height),
	  parNum(parNum),
	  parDen(parDen),
	  imageFormat(imageFormat)
    {}
    int width;
    int height;
    int parNum;  // pixel aspect ratio numerator
    int parDen;  // pixel aspect ratio denominator
    int imageFormat;
};

// ===================================================================

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

struct NoAudioStream {};
struct NoVideoStream {};

// ===================================================================

class XFVideoImage;

struct DeleteXFVideoImage
{
    DeleteXFVideoImage(boost::shared_ptr<XFVideoImage> image)
	: image(image)
    {}

    boost::shared_ptr<XFVideoImage> image;
};

// ===================================================================

struct EndOfSystemStream {};
struct EndOfAudioStream {};
struct EndOfVideoStream {};

// ===================================================================

struct CommandPlay{};
struct CommandPause{};

struct CommandRewind{};
struct CommandForward{};

struct CommandSetPlaybackVolume
{
    CommandSetPlaybackVolume(long volume)
	: volume(volume)
    {}
    long volume;
};

struct CommandSetPlaybackSwitch
{
    CommandSetPlaybackSwitch(bool enabled)
	: enabled(enabled)
    {}
    bool enabled;
};

// ===================================================================

struct NotificationFileInfo
{
    std::string fileName;
    double duration;
    int64_t file_size;
};

struct NotificationCurrentTime
{
    NotificationCurrentTime(double time)
	: time(time)
    {}
    double time;
};

struct NotificationCurrentVolume
{
    std::string name;
    long volume;
    bool enabled;
    long minVolume;
    long maxVolume;
};

struct NotificationVideoSize
{
    enum Reason {
	VideoSizeChanged,
	WindowSizeChanged,
	ClippingChanged
    };
    Reason reason;

    // size of the video
    unsigned int widthVid;
    unsigned int heightVid;

    // window size:
    unsigned int widthWin;
    unsigned int heightWin;

    // sub area of window displaying the video:
    unsigned int leftDst;
    unsigned int topDst;
    unsigned int widthDst;
    unsigned int heightDst;

    // sub area of video shown on display:
    unsigned int leftSrc;
    unsigned int topSrc;
    unsigned int widthSrc;
    unsigned int heightSrc;

    // size for 100% zoom of clipped area. This may be different 
    // from (widthSrc, heightSrc) to get correct aspect ratio:
    unsigned int widthAdj;
    unsigned int heightAdj;
};

struct NotificationClipping
{
    // Values are video pixels.

    NotificationClipping(int left, int right, int top, int bottom)
	: left(left),
	  right(right),
	  top(top),
	  bottom(bottom)
    {}

    int left;
    int right;
    int top;
    int bottom;
};

struct NotificationDeinterlacerList
{
    int selected;   // -1: none
    std::list<std::string> list;
};

// ===================================================================

struct HideCursorEvent {};

struct WindowRealizeEvent
{
    WindowRealizeEvent(void* display, unsigned long window)
	: display(display),
	  window(window)
    {}
    void* display;
    unsigned long window;
};

struct WindowConfigureEvent
{
    WindowConfigureEvent(int x, int y, int width, int height)
	: x(x), y(y), width(width), height(height)
    {}
    int x;
    int y;
    int width;
    int height;
};

struct WindowExposeEvent
{
    // GdkRectangle area;
    //    GdkRegion *region;
    //    gint count;
};

struct ClipVideoDstEvent
{
    // Values are window pixels, i.e. values after zooming.
    // except the special values:
    // -1: keep clipping as it is at this edge
    // -2: no clipping at this edge

    ClipVideoDstEvent(int val)
	: left(val),
	  right(val),
	  top(val),
	  bottom(val)
    {}

    ClipVideoDstEvent(int left, int right, int top, int bottom)
	: left(left),
	  right(right),
	  top(top),
	  bottom(bottom)
    {}

    int left;
    int right;
    int top;
    int bottom;

    inline static ClipVideoDstEvent* createKeepIt()
    {
	return new ClipVideoDstEvent(-1);
    }

    inline static ClipVideoDstEvent* createDisable()
    {
	return new ClipVideoDstEvent(-2);
    }
};

struct ClipVideoSrcEvent
{
    // Values are video pixels.

    ClipVideoSrcEvent(int left, int right, int top, int bottom)
	: left(left),
	  right(right),
	  top(top),
	  bottom(bottom)
    {}

    int left;
    int right;
    int top;
    int bottom;
};

// ===================================================================

struct EnableOptimalPixelFormat {};
struct DisableOptimalPixelFormat {};

struct EnableXvClipping {};
struct DisableXvClipping {};

struct SelectDeinterlacer
{
    SelectDeinterlacer(const std::string& name)
	: name(name)
    {}
    std::string name;
};

// ===================================================================

#ifndef SYNCTEST

class MediaPlayerThreadNotification
{
public:
    typedef void (*fct_t)();

    MediaPlayerThreadNotification();
    static void setCallback(fct_t fct);

private:
    static fct_t m_fct;
};

#endif

// ===================================================================

#ifdef SYNCTEST
#include "SyncTest.hpp"
#endif

#endif
