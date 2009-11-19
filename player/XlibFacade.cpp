//
// X11 and Xv Extension Interface
//
// Copyright (C) Joachim Erbs, 2009
//

#include "player/XlibFacade.hpp"
#include "player/XlibHelpers.hpp"
#include "platform/Logging.hpp"

#include <sys/ipc.h>  // to allocate shared memory
#include <sys/shm.h>  // to allocate shared memory
#include <errno.h>
#include <string.h>   // strerror

#undef DEBUG
#define DEBUG(s)

using namespace std;

// -------------------------------------------------------------------

XFVideo::XFVideo(Display* display, Window window,
		 unsigned int width, unsigned int height)
    : m_display(display),
      m_window(window),
      yuvWidth(width),
      yuvHeight(height),
      ratio(double(width)/double(height)),
      iratio(double(height)/double(width))
{
    // Check to see if the shared memory extension is available:
    bool shmExtAvailable = XShmQueryExtension(m_display);
    if (!shmExtAvailable)
    {
	THROW(XFException, << "No shared memory extension available.");
    }

    // Check to see if Xv extension is available:
    XvVersionInfo xvi;
    switch(XvQueryExtension(m_display,
			    &xvi.version,
			    &xvi.revision,
			    &xvi.request_base,
			    &xvi.event_base,
			    &xvi.error_base))
    {
    case Success:
	break;
    case XvBadExtension:
    case XvBadAlloc:
    default:
	THROW(XFException, << "No Xv extension available.");
    }

    // Querry XvAdapterInfo:
    unsigned int num_ai;
    XvAdaptorInfo* ai;
    switch(XvQueryAdaptors(m_display,
			   DefaultRootWindow(m_display),
			   &num_ai, &ai))
    {
    case Success:
	break;
    case XvBadExtension:
    case XvBadAlloc:
    default:
	THROW(XFException, << "XvQueryAdaptors failed.");
    }

    xvPortId = INVALID_XV_PORT_ID;
    imageFormat = 0;
    
    for (unsigned int i=0; i<num_ai; i++)
    {
	DEBUG( << "XvAdaptorInfo[" << i << "]:\n" << ai[i] );

	for (XvPortID adaptorPort = ai[i].base_id;
	     adaptorPort < ai[i].base_id+ai[i].num_ports;
	     adaptorPort++)
	{
	    if (xvPortId == INVALID_XV_PORT_ID)
	    {
		if ((ai[i].type & XvInputMask) &&
		    (ai[i].type & XvImageMask))
		{
		    switch(XvGrabPort( m_display, adaptorPort, CurrentTime ))
		    {
		    case Success:
			xvPortId = adaptorPort;
			break;
		    default:
			ERROR( << "XvGrabPort failed for adaptorPort " 
			       << adaptorPort << "." );
		    }
		}
	    }

	    // Querry XvEncodingInfo:
	    unsigned int num_ei;
	    XvEncodingInfo* ei;
	    switch(XvQueryEncodings(m_display, adaptorPort,
				    &num_ei, &ei))
	    {
	    case Success:
		break;
	    default:
		THROW(XFException, << "XvQueryEncodings failed.");
	    }

	    for (unsigned int j=0; j<num_ei; j++)
	    {
		DEBUG( << "XvEncodingInfo[" << j << "]:\n" << ei[j] );
	    }

	    XvFreeEncodingInfo(ei);

	    // Querry XvAttributes
	    int num_att;
	    XvAttribute* att = XvQueryPortAttributes(m_display, adaptorPort, &num_att);
	    if (att)
	    {
		for (int k=0; k<num_att; k++)
		{
		    DEBUG( << "XvAttribute[" << k << "]\n" << att[k] );
		}
		XFree(att);
	    }

	    // Querry XvImageFormatValues
	    int num_ifv;
	    XvImageFormatValues* ifv = XvListImageFormats(m_display, adaptorPort, &num_ifv);
	    if (ifv)
	    {
		for (int l=0; l<num_ifv; l++)
		{
		    DEBUG( << "XvImageFormatValues[" << l << "]\n" << ifv[l] );
		    // if (ifv[l].id == GUID_UYVY_PLANAR)
		    if (ifv[l].id == GUID_YUV12_PLANAR)
		    {
			imageFormat = ifv[l].id;
		    }
		}
		XFree(ifv);
	    }

	}
    }

    XvFreeAdaptorInfo(ai);

    if (xvPortId == INVALID_XV_PORT_ID)
    {
	THROW(XFException, << "No XvAdaptorPort found.");
    }

    if (!imageFormat)
    {
	THROW(XFException, << "Needed image format not found.");
    }

    // Create a graphics context
    gc = XCreateGC(display, window, 0, 0);

    calculateDestinationArea();
    paintBorder();
}

XFVideo::~XFVideo()
{
    XFreeGC(m_display, gc);
    XvUngrabPort(m_display, xvPortId, CurrentTime );
}

void XFVideo::selectEvents()
{
    // Select events:
    XSelectInput(m_display, m_window, StructureNotifyMask | ExposureMask | KeyPressMask);
}

void XFVideo::resize(unsigned int width_, unsigned int height_)
{
    width  = width_;
    height = height_;
    yuvWidth = width;
    yuvHeight = height;
    ratio = double(width)/double(height);
    iratio = double(height)/double(width);

    XResizeWindow(m_display, m_window, width, height);
    // XMoveResizeWindow(m_display, m_window, left, top, width, height);

    calculateDestinationArea();
}

void XFVideo::calculateDestinationArea()
{
    Window root;
    int x, y;
    unsigned int border_width, depth;

    XGetGeometry(m_display, m_window, &root,
		 &x, &y, &width, &height,
		 &border_width, &depth);

    topDest = 0;
    leftDest = 0;

    // Keep aspect ratio:
    widthDest = ratio * double(height);
    heightDest = iratio * double(width);

    if (widthDest<=width)
    {
	heightDest = height;
	leftDest = (width-widthDest)>>1;
    }
    else if (heightDest<=height)
    {
	widthDest = width;
	topDest = (height-heightDest)>>1;
    }
    else
    {
	heightDest = height;
	widthDest = width;
    }
}

boost::shared_ptr<XFVideoImage> XFVideo::show(boost::shared_ptr<XFVideoImage> yuvImage)
{    
    boost::shared_ptr<XFVideoImage> previousImage = m_displayedImage;
    m_displayedImage = yuvImage;

    show();

    return previousImage;
}

void XFVideo::show()
{
    if (m_displayedImage)
    {
	// Display srcArea of yuvImage on destArea of Drawable window:
	XvShmPutImage(m_display, xvPortId, m_window, gc, m_displayedImage->xvImage(),
		      0, 0, m_displayedImage->width(), m_displayedImage->height(), // srcArea  (x,y,w,h)
		      leftDest, topDest, widthDest, heightDest,    // destArea (x,y,w,h)
		      True);
    
	// Explicitly calling XFlush to ensure that the image is visible:
	XFlush(m_display);
    }
}

void XFVideo::handleConfigureEvent()
{
    calculateDestinationArea();
}

void XFVideo::handleExposeEvent()
{
    paintBorder();
    show();
}

void XFVideo::paintBorder()
{
    //     x1   x2   x3
    //  y1 +----+----+----+
    //     | R1 | R2 | R3 | h1
    //  y2 +----+----+----+
    //     | R4 |    | R5 | h2
    //  y3 +----+----+----+
    //     | R6 | R7 | R8 | h3
    //     +----+----+----+
    //       w1   w2   w3

    const short x1 = 0;
    const short x2 = leftDest;
    const short x3 = leftDest+widthDest;

    const short y1 = 0;
    const short y2 = topDest;
    const short y3 = topDest+heightDest;

    const unsigned short w1 = leftDest;
    const unsigned short w2 = widthDest;
    const unsigned short w3 = width-leftDest-widthDest;

    const unsigned short h1 = topDest;
    const unsigned short h2 = heightDest;
    const unsigned short h3 = height-topDest-heightDest;

    XRectangle rec[8] = { {x1, y1, w1, h1},   // R1
			  {x2, y1, w2, h1},   // R2
			  {x3, y1, w3, h1},   // R3
			  {x1, y2, w1, h2},   // R4
			  {x3, y2, w3, h2},   // R5
			  {x1, y3, w1, h3},   // R6
			  {x2, y3, w2, h3},   // R7
			  {x3, y3, w3, h3} }; // R8

    XFillRectangles(m_display, m_window, gc, rec, 8);
}
 
// -------------------------------------------------------------------

XFVideoImage::XFVideoImage(boost::shared_ptr<XFVideo> xfVideo)
    : pts(0)
{
    yuvImage = XvShmCreateImage(xfVideo->display(),
				xfVideo->xvPortId,
				xfVideo->imageFormat,
				0, // char* data
				xfVideo->yuvWidth, xfVideo->yuvHeight,
				&yuvShmInfo);

    DEBUG(<< "requested: " << xfVideo->yuvWidth << "*" << xfVideo->yuvHeight
	  << " got: " << yuvImage->width << "*" << yuvImage->height);

    // Allocate the shared memory requested by the server:
    yuvShmInfo.shmid = shmget(IPC_PRIVATE,
			      yuvImage->data_size,
			      IPC_CREAT | 0777);
    if (yuvShmInfo.shmid == -1)
    {
	THROW(XFException,
	      << "shmget failed"
	      << " data_size=" << yuvImage->data_size
	      << " errno=" << strerror(errno) << "(" << errno << ")");
    }

    // Attach the shared memory to the address space of the calling process:
    shmAddr = shmat(yuvShmInfo.shmid, 0, 0);
    if (shmAddr == (void*)-1)
    {
	THROW(XFException,
	      << "shmat failed"
	      << " yuvShmInfo.shmid=" << yuvShmInfo.shmid 
	      << " data_size=" << yuvImage->data_size
	      << " errno=" << strerror(errno) << "(" << errno << ")");
    }

    // Update XShmSegmentInfo and XvImage with pointer to shared memory:
    yuvImage->data = (char*)shmAddr;
    yuvShmInfo.shmaddr = (char*)shmAddr;
    yuvShmInfo.readOnly = False;
  
    // Attach the shared memory to the X server:
    if (!XShmAttach(xfVideo->display(), &yuvShmInfo))
    {
	THROW(XFException, << "XShmAttach failed !");
    }

    DEBUG( "yuvImage data_size = " << std::dec << yuvImage->data_size );
}

XFVideoImage::~XFVideoImage()
{
    // Detach shared memory from the address space of the calling process:
    shmdt(shmAddr);

    // Mark the shared memory segment to be destroyed.
    // The invers operation of shmget:
    shmctl(yuvShmInfo.shmid, IPC_RMID, 0);

    XFree(yuvImage);
}

void XFVideoImage::createBlackImage()
{
    // YUV in fact is YCbCr.
    // http://en.wikipedia.org/wiki/YCbCr

    // Y'CbCr (601) from R'G'B'
    // ========================================================
    // Y' =  16 + ( 65.481 * R' + 128.553 * G' +  24.966 * B')
    // Cb = 128 + (-37.797 * R' -  74.203 * G' + 112.0   * B')
    // Cr = 128 + (112.0   * R' -  93.786 * G' -  18.214 * B')
    // ........................................................
    // R', G', B' in [0; 1]
    // Y'        in {16, 17, ..., 235}
    //    with footroom in {1, 2, ..., 15}
    //    headroom in {236, 237, ..., 254}
    //    sync.  in {0, 255}
    // Cb, Cr      in {16, 17, ..., 240}

    int w = width();
    int h = height();

    char* Y = data();
    char* V = Y + w * h;
    char* U = V + w/2 * h/2;

    // Y = 16 isn't really black.
    memset(Y, 0,   w   * h);
    memset(V, 128, w/2 * h/2);
    memset(U, 128, w/2 * h/2);
}

void XFVideoImage::createDemoImage()
{
    int w = width();
    int h = height();

    char* Y = data();
    char* V = Y + w * h;
    char* U = V + w/2 * h/2;

    memset(Y, 0, w * h);
    memset(V, 0, w/2 * h/2);
    memset(U, 0, w/2 * h/2);

    for (int x=0; x<w; x++)
	for (int y=0; y<h; y++)
	{
	    Y[x + y*w] = x-y;
	}
}

// -------------------------------------------------------------------
