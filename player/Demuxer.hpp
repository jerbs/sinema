//
// Demultiplexer
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef DEMUXER_HPP
#define DEMUXER_HPP

#include "player/GeneralEvents.hpp"
#include "player/SystemStreamEvents.hpp"
#include "platform/event_receiver.hpp"

#include <boost/shared_ptr.hpp>

class Demuxer : public event_receiver<Demuxer>
{
    // The friend declaration allows to define the process methods private:
    friend class event_processor<>;

    boost::shared_ptr<event_processor<> > m_event_processor;

    AVFormatContext* avFormatContext;

    enum SystemStreamStatus {
	SystemStreamClosed,
	SystemStreamOpening,
	SystemStreamOpened,
	SystemStreamFailed,
	SystemStreamClosing
    };
    SystemStreamStatus systemStreamStatus;

    int audioStreamIndex;
    int videoStreamIndex;

    enum StreamStatus {
	StreamClosed,
	StreamOpening,
	StreamOpened,
	StreamClosing
    };
    StreamStatus audioStreamStatus;
    StreamStatus videoStreamStatus;

    static Demuxer* obj;

    int queuedAudioPackets;
    int queuedVideoPackets;
    int targetQueuedAudioPackets;
    int targetQueuedVideoPackets;

public:
    Demuxer(event_processor_ptr_type evt_proc);
    ~Demuxer();

    // Custom main loop for event_processor:
    void operator()();

private:
    boost::shared_ptr<FileReader> fileReader;
    boost::shared_ptr<AudioDecoder> audioDecoder;
    boost::shared_ptr<VideoDecoder> videoDecoder;

    static int interrupt_cb();

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<StopEvent> event);

    void process(boost::shared_ptr<SystemStreamChunkEvent> event);

    void process(boost::shared_ptr<OpenFileEvent> event);
    void process(boost::shared_ptr<OpenAudioStreamResp> event);
    void process(boost::shared_ptr<OpenAudioStreamFail> event);
    void process(boost::shared_ptr<OpenVideoStreamResp> event);
    void process(boost::shared_ptr<OpenVideoStreamFail> event);

    void process(boost::shared_ptr<CloseFileEvent> event);
    void process(boost::shared_ptr<CloseAudioStreamResp> event);
    void process(boost::shared_ptr<CloseVideoStreamResp> event);

    void process(boost::shared_ptr<ConfirmAudioPacketEvent> event);
    void process(boost::shared_ptr<ConfirmVideoPacketEvent> event);

    void updateSystemStreamStatusOpening();
    void updateSystemStreamStatusClosing();
};

#endif
