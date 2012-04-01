//
// Media Player
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

#ifndef MEDIA_PLAYER_HPP
#define MEDIA_PLAYER_HPP

#include "platform/Logging.hpp"
#include "platform/event_receiver.hpp"
#include "player/GeneralEvents.hpp"

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

class Demuxer;
class VideoDecoder;
class AudioDecoder;
class VideoOutput;
class AudioOutput;
class Deinterlacer;
class PlayList;

class MediaPlayer : public event_receiver<MediaPlayer,
					  concurrent_queue<receive_fct_t, with_callback_function> >
{
    // The friend declaration allows to define the process methods private:
    friend class event_processor<concurrent_queue<receive_fct_t, with_callback_function> >;

public:
    MediaPlayer(PlayList& playList);
    ~MediaPlayer();

    void init();

    void open();
    void close();

    void play();
    void pause();

    void skipBack();
    void skipForward();

    void seekAbsolute(double second);
    void seekRelative(double secondsDelta);

    void setPlaybackVolume(double volume);
    void setPlaybackSwitch(bool enabled);

    void clip(boost::shared_ptr<ClipVideoDstEvent> event);
    void clip(boost::shared_ptr<ClipVideoSrcEvent> event);

    void enableOptimalPixelFormat();
    void disableOptimalPixelFormat();

    void enableXvClipping();
    void disableXvClipping();

    void selectDeinterlacer(const std::string& name);

    void setVideoAttribute(const std::string& name, int value);

protected:
    // EventReceiver
    boost::shared_ptr<Demuxer> demuxer;
    boost::shared_ptr<VideoDecoder> videoDecoder;
    boost::shared_ptr<AudioDecoder> audioDecoder;
    boost::shared_ptr<VideoOutput> videoOutput;
    boost::shared_ptr<AudioOutput> audioOutput;
    boost::shared_ptr<Deinterlacer> deinterlacer;

private:
    // Boost threads:
    boost::thread demuxerThread;
    boost::thread decoderThread;
    boost::thread outputThread;
    // boost::thread videoDecoderThread;
    // boost::thread audioDecoderThread;
    // boost::thread videoOutputThread;
    // boost::thread audioOutputThread;

    // EventProcessor:
    boost::shared_ptr<event_processor<> > demuxerEventProcessor;
    boost::shared_ptr<event_processor<> > decoderEventProcessor;
    boost::shared_ptr<event_processor<> > outputEventProcessor;

    // PlayList:
    PlayList& m_PlayList;

    void sendInitEvents();

    void process(boost::shared_ptr<OpenFileResp> event);
    void process(boost::shared_ptr<OpenFileFail> event);
    virtual void process(boost::shared_ptr<CloseFileResp> event);

    void process(boost::shared_ptr<NoAudioStream> event);
    void process(boost::shared_ptr<NoVideoStream> event);

    void process(boost::shared_ptr<EndOfSystemStream> event);
    void process(boost::shared_ptr<EndOfAudioStream> event);
    void process(boost::shared_ptr<EndOfVideoStream> event);

    void process(boost::shared_ptr<AudioSyncInfo> event);

    bool skipForwardInt();

    bool hasAudioStream;
    bool hasVideoStream;

    bool endOfAudioStream;
    bool endOfVideoStream;

    virtual void process(boost::shared_ptr<NotificationFileInfo> event) = 0;
    virtual void process(boost::shared_ptr<NotificationCurrentTime> event) = 0;
    virtual void process(boost::shared_ptr<NotificationCurrentVolume> event) = 0;
    virtual void process(boost::shared_ptr<NotificationVideoSize> event) = 0;
    virtual void process(boost::shared_ptr<NotificationClipping> event) = 0;
    virtual void process(boost::shared_ptr<NotificationDeinterlacerList> event) = 0;
    virtual void process(boost::shared_ptr<NotificationVideoAttribute> event) = 0;

    virtual void process(boost::shared_ptr<OpenAudioStreamFailed>) {};
    virtual void process(boost::shared_ptr<OpenVideoStreamFailed>) {};

    virtual void process(boost::shared_ptr<HideCursorEvent> event) = 0;
};

#endif
