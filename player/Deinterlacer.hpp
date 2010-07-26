//
// Deinterlacer
//
// Copyright (C) Joachim Erbs, 2010
//

#ifndef DEINTERLACER_HPP
#define DEINTERLACER_HPP

#include "player/GeneralEvents.hpp"
#include "platform/event_receiver.hpp"

#include <boost/shared_ptr.hpp>
#include <list>

class XFVideoImage;
struct deinterlace_method_s;
typedef struct deinterlace_method_s deinterlace_method_t;

struct TopFieldFirst {};
struct BottomFieldFirst {};

class Deinterlacer : public event_receiver<Deinterlacer>
{
    friend class event_processor<>;

    std::queue<std::unique_ptr<XFVideoImage> > m_emptyImages;
    std::list<std::unique_ptr<XFVideoImage> > m_interlacedImages;
    bool m_topFieldFirst;
    deinterlace_method_t * m_deinterlacer;
    bool m_nextImageHasContent;
    bool m_topField;

public:
    Deinterlacer(event_processor_ptr_type evt_proc);
    ~Deinterlacer();

private:
    MediaPlayer* mediaPlayer;
    boost::shared_ptr<VideoOutput> videoOutput;
    boost::shared_ptr<VideoDecoder> videoDecoder;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<CloseVideoOutputReq> event);

    void process(  std::unique_ptr<XFVideoImage> event);
    void process(boost::shared_ptr<TopFieldFirst> event);
    void process(boost::shared_ptr<BottomFieldFirst> event);

    void process(boost::shared_ptr<FlushReq> event);
    void process(boost::shared_ptr<EndOfVideoStream> event);

    void process(boost::shared_ptr<SelectDeinterlacer> event);

    void deinterlace();

    void announceDeinterlacers();
};

#endif
