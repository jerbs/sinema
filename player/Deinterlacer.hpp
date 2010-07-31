//
// Deinterlacer
//
// Copyright (C) Joachim Erbs, 2010
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
