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

    std::queue<boost::shared_ptr<XFVideoImage> > m_emptyImages;
    std::list<boost::shared_ptr<XFVideoImage> > m_interlacedImages;
    bool m_topFieldFirst;
    deinterlace_method_t * m_deinterlacer;
    bool m_nextImageHasContent;
    bool m_topField;

public:
    Deinterlacer(event_processor_ptr_type evt_proc);
    ~Deinterlacer();

private:
    boost::shared_ptr<VideoOutput> videoOutput;

    void process(boost::shared_ptr<InitEvent> event);
    void process(boost::shared_ptr<XFVideoImage> event);
    void process(boost::shared_ptr<TopFieldFirst> event);
    void process(boost::shared_ptr<BottomFieldFirst> event);

    void deinterlace();
};

#endif
