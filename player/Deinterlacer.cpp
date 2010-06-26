//
// Deinterlacer
//
// Copyright (C) Joachim Erbs, 2010
//

#include "player/Deinterlacer.hpp"
#include "player/VideoOutput.hpp"
#include "player/VideoDecoder.hpp"
#include "player/XlibFacade.hpp"
#include "player/XlibHelpers.hpp"

#include "deinterlacer/plugins/plugins.h"
#include "deinterlacer/src/deinterlace.h"
#include "deinterlacer/src/copyfunctions.h"
#include "deinterlacer/src/mm_accel.h"

// #undef DEBUG
// #define DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

Deinterlacer::Deinterlacer(event_processor_ptr_type evt_proc)
    : base_type(evt_proc),
      m_topFieldFirst(true),
      m_nextImageHasContent(true),
      m_topField(true)
{
    setup_copyfunctions(MM_ACCEL_X86_MMXEXT);
    m_deinterlacer = greedy_get_method();
}

Deinterlacer::~Deinterlacer()
{
}

void Deinterlacer::process(boost::shared_ptr<InitEvent> event)
{
    DEBUG(<< "tid = " << gettid());
    videoOutput = event->videoOutput;
    videoDecoder = event->videoDecoder;
}

void Deinterlacer::process(boost::shared_ptr<CloseVideoOutputReq> event)
{
    // Throw away all queued empty frames:
    while(!m_emptyImages.empty())
    {
	boost::shared_ptr<XFVideoImage> xfVideoImage(m_emptyImages.front());
	m_emptyImages.pop();
	videoOutput->queue_event(boost::make_shared<DeleteXFVideoImage>(xfVideoImage));
    }

    // Throw away all queued interlaced images:
    while(!m_interlacedImages.empty())
    {
	boost::shared_ptr<XFVideoImage> xfVideoImage(m_interlacedImages.front());
	m_interlacedImages.pop_front();
	videoOutput->queue_event(boost::make_shared<DeleteXFVideoImage>(xfVideoImage));
    }

    // Reset member data:
    m_topFieldFirst = true;
    m_nextImageHasContent = true;
    m_topField = true;

    // Forward CloseVideoOutputReq to VideoOutput:
    videoOutput->queue_event(event);
}

void Deinterlacer::process(boost::shared_ptr<XFVideoImage> event)
{
    DEBUG(<< m_interlacedImages.size() << ", " << m_emptyImages.size());

    if (m_nextImageHasContent)
	m_interlacedImages.push_back(event);
    else
	m_emptyImages.push(event);

    m_nextImageHasContent = !m_nextImageHasContent;

    if ( m_interlacedImages.size() >= 3 &&
	 !m_emptyImages.empty() )
    {
	deinterlace();
    }
}

void Deinterlacer::process(boost::shared_ptr<TopFieldFirst> event)
{
    DEBUG();
    m_topFieldFirst = true;

    // Continue with top field:
    m_topField = true;
}

void Deinterlacer::process(boost::shared_ptr<BottomFieldFirst> event)
{
    DEBUG();
    m_topFieldFirst = false;

    // Continue with bottom field:
    m_topField = false;
}

void Deinterlacer::process(boost::shared_ptr<FlushReq> event)
{
    // The XFVideoImage objects are still needed, do not throw them away here.
    // Here seek is used to jump to another file position.

    // Remove everything from the incoming queues without showing it on the
    // output device.

    // Send all queued empty frames pack to VideoDecoder:
    while(!m_emptyImages.empty())
    {
	boost::shared_ptr<XFVideoImage> image(m_emptyImages.front());
	m_emptyImages.pop();
	videoDecoder->queue_event(image);
    }

    // Send all queued interlaced images pack to VideoDecoder:
    while(!m_interlacedImages.empty())
    {
	boost::shared_ptr<XFVideoImage> image(m_interlacedImages.front());
	m_interlacedImages.pop_front();
	videoDecoder->queue_event(image);
    }

    // Do not reset member data here.

    // Forward FlushReq to VideoOutput:
    videoOutput->queue_event(event);
}

void Deinterlacer::process(boost::shared_ptr<EndOfVideoStream> event)
{
    // The XFVideoImage objects are still needed, do not throw them away here.
    // It is still posible that seek is used to jump to another file position.

    // Remove everything from the incoming queues and show it on the
    // output device.

    // Send all queued empty frames pack to VideoDecoder:
    while(!m_emptyImages.empty())
    {
	boost::shared_ptr<XFVideoImage> image(m_emptyImages.front());
	m_emptyImages.pop();
	videoDecoder->queue_event(image);
    }

    // Forward all queued interlaced images to VideoOutput. It is not possible
    // to deinterlace the last frames:
    while(!m_interlacedImages.empty())
    {
	boost::shared_ptr<XFVideoImage> image(m_interlacedImages.front());
	m_interlacedImages.pop_front();
	videoOutput->queue_event(image);
    }

    // Finally forward EndOfVideoStream indication to VideoOutput:
    videoOutput->queue_event(event);
}

// -------------------------------------------------------------------

static inline uint8_t* getLineAddr(XvImage* yuvImage, int line)
{
    char* Packed = yuvImage->data + yuvImage->offsets[0];
    char* Line = Packed + line * yuvImage->pitches[0];
    return (uint8_t*)Line;
    
}

// -------------------------------------------------------------------
// Line offsets used for the scan line mode:

template<int n>
struct LineOffsets;

template<>
struct LineOffsets<-2>   // top most line
{
    enum {tt =  2};
    enum {t  =  1};
    enum {m  =  0};
    enum {b  =  1};
    enum {bb =  2};
};

template<>
struct LineOffsets<-1>   // second line
{
    enum {tt =  0};
    enum {t  = -1};
    enum {m  =  0};
    enum {b  =  1};
    enum {bb =  2};
};

template<>
struct LineOffsets<0>    // all lines in the middle
{
    enum {tt = -2};
    enum {t  = -1};
    enum {m  =  0};
    enum {b  =  1};
    enum {bb =  2};
};


template<>
struct LineOffsets<1>   // second last line
{
    enum {tt = -2};
    enum {t  = -1};
    enum {m  =  0};
    enum {b  =  1};
    enum {bb =  0};
};

template<>
struct LineOffsets<2>   // bottom most line
{
    enum {tt = -2};
    enum {t  = -1};
    enum {m  =  0};
    enum {b  = -1};
    enum {bb = -2};
};


// -------------------------------------------------------------------
// For scan line mode:

template<int n>
static inline void copyLine(deinterlace_copy_scanline_t copy, int line, XvImage* yuvImage,
			    XvImage* field0, XvImage* field1, XvImage* field2, XvImage* field3,
			    bool bottomField)
{
    uint8_t* out = getLineAddr(yuvImage, line);
    uint8_t* tt0 = getLineAddr(field0, line + LineOffsets<n>::tt);
    uint8_t* m0  = getLineAddr(field0, line);
    uint8_t* bb0 = getLineAddr(field0, line + LineOffsets<n>::bb);
    uint8_t* t1  = getLineAddr(field1, line + LineOffsets<n>::t);
    uint8_t* b1  = getLineAddr(field1, line + LineOffsets<n>::b);
    uint8_t* tt2 = getLineAddr(field2, line + LineOffsets<n>::tt);
    uint8_t* m2  = getLineAddr(field2, line);
    uint8_t* bb2 = getLineAddr(field2, line + LineOffsets<n>::bb);
    uint8_t* t3  = getLineAddr(field3, line + LineOffsets<n>::t);
    uint8_t* b3  = getLineAddr(field3, line + LineOffsets<n>::b);
    deinterlace_scanline_data_s data = {tt0,  0, m0,  0,bb0,
					0,   t1,  0, b1,  0,
					tt2,  0, m2,  0,bb2,
					0,   t3,  0, b3,  0,
					bottomField};
    copy(out, &data, yuvImage->width);
}

template<int n>
static inline void intpLine(deinterlace_interp_scanline_t intp, int line, XvImage* yuvImage,
			    XvImage* field0, XvImage* field1, XvImage* field2, XvImage* field3,
			    bool bottomField)
{
    uint8_t* out = getLineAddr(yuvImage, line);
    uint8_t* t0  = getLineAddr(field0, line + LineOffsets<n>::t);
    uint8_t* b0  = getLineAddr(field0, line + LineOffsets<n>::b);
    uint8_t* tt1 = getLineAddr(field1, line + LineOffsets<n>::tt);
    uint8_t* m1  = getLineAddr(field1, line);
    uint8_t* bb1 = getLineAddr(field1, line + LineOffsets<n>::bb);
    uint8_t* t2  = getLineAddr(field2, line + LineOffsets<n>::t);
    uint8_t* b2  = getLineAddr(field2, line + LineOffsets<n>::b);
    uint8_t* tt3 = getLineAddr(field3, line + LineOffsets<n>::tt);
    uint8_t* m3  = getLineAddr(field3, line);
    uint8_t* bb3 = getLineAddr(field3, line + LineOffsets<n>::bb);
    deinterlace_scanline_data_s data = {0,   t0,  0, b0,  0,
					tt1,  0, m1,  0,bb1,
					0,   t2,  0, b2,  0,
					tt3,  0, m3,  0,bb3,
					bottomField};
    intp(out, &data, yuvImage->width);
}

// -------------------------------------------------------------------

void Deinterlacer::deinterlace()
{
    DEBUG();
    boost::shared_ptr<XFVideoImage> image(m_emptyImages.front());
    m_emptyImages.pop();

    XvImage* yuvImage = image->xvImage();
    int w = yuvImage->width;
    int h = yuvImage->height;
    char* Packed = yuvImage->data + yuvImage->offsets[0];

    std::list<boost::shared_ptr<XFVideoImage> >::iterator it = m_interlacedImages.begin();

    double pts;

    deinterlace_interp_scanline_t intp = m_deinterlacer->interpolate_scanline;
    deinterlace_copy_scanline_t   copy = m_deinterlacer->copy_scanline;

    XvImage *field0, *field1, *field2, *field3;

    if (m_topField == m_topFieldFirst)
    {
	pts = (*it)->getPTS();
	field0 = (*it)->xvImage();
	field1 = (*it)->xvImage();
	it++;
	field2 = (*it)->xvImage();
	field3 = (*it)->xvImage();
    }
    else
    {
	pts = (*it)->getPTS();
	field0 = (*it)->xvImage();
	it++;
	pts = (pts + (*it)->getPTS()) / 2;
	field1 = (*it)->xvImage();
	field2 = (*it)->xvImage();
	it++;
	field3 = (*it)->xvImage();
    }

    if (m_topField)
    {
	// Field 0 is a top field:
	int line = 0;
	copyLine<-2>(copy, line, yuvImage, field0, field1, field2, field3, false);
	line++;
	intpLine<-1>(intp, line, yuvImage, field0, field1, field2, field3, false);
	line++;

	while (line < h-3)
	{
	    copyLine<0>(copy, line, yuvImage, field0, field1, field2, field3, false);
	    line++;
	    intpLine<0>(intp, line, yuvImage, field0, field1, field2, field3, false);
	    line++;
	}

	if (line == h-2)
	{
	    copyLine<1>(copy, line, yuvImage, field0, field1, field2, field3, false);
	    intpLine<2>(intp, line, yuvImage, field0, field1, field2, field3, false);
	}
	else
	{
	    copyLine<0>(copy, line, yuvImage, field0, field1, field2, field3, false);
	    intpLine<1>(intp, line, yuvImage, field0, field1, field2, field3, false);
	    copyLine<2>(copy, line, yuvImage, field0, field1, field2, field3, false);
	}
    }
    else
    {
	// Field 0 is a bottom field:
	int line = 0;
	intpLine<-2>(intp, line, yuvImage, field0, field1, field2, field3, true);
	line++;
	copyLine<-1>(copy, line, yuvImage, field0, field1, field2, field3, true);
	line++;

	while (line < h-3)
	{
	    intpLine<0>(intp, line, yuvImage, field0, field1, field2, field3, true);
	    line++;
	    copyLine<0>(copy, line, yuvImage, field0, field1, field2, field3, true);
	    line++;
	}

	if (line == h-2)
	{
	    intpLine<1>(intp, line, yuvImage, field0, field1, field2, field3, true);
	    copyLine<2>(copy, line, yuvImage, field0, field1, field2, field3, true);
	}
	else
	{
	    intpLine<0>(intp, line, yuvImage, field0, field1, field2, field3, true);
	    copyLine<1>(copy, line, yuvImage, field0, field1, field2, field3, true);
	    intpLine<2>(intp, line, yuvImage, field0, field1, field2, field3, true);
	}
    }

    if (m_topField != m_topFieldFirst)
    {
	// Processed both fields of the image.
	boost::shared_ptr<XFVideoImage> img(m_interlacedImages.front());
	m_interlacedImages.pop_front();
	m_emptyImages.push(img);
    }

    image->setPTS(pts);
    videoOutput->queue_event(image);

    m_topField = !m_topField;
}