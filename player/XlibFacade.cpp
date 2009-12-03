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
#include <string.h>   // memcpy, strerror
#include <math.h>

#undef DEBUG
#define DEBUG(s)

#undef INFO
#define INFO(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

using namespace std;

bool useXvClipping = false;

// -------------------------------------------------------------------

XFVideo::XFVideo(Display* display, Window window,
		 unsigned int width, unsigned int height,
		 send_notification_video_size_fct_t fct)
    : m_display(display),
      m_window(window),
      widthVid(width),
      heightVid(height),
      widthWin(width),
      heightWin(height),
      leftSrc(0),
      topSrc(0),
      widthSrc(width),
      heightSrc(height),
      sendNotificationVideoSize(fct)
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

void XFVideo::resize(unsigned int width, unsigned int height)
{
    // This method is called when the video size changes.
    // It is not called when the widget is resized.
    INFO(<< width << "," << height);

    // Display the full image:
    leftSrc = 0;
    topSrc = 0;
    widthVid  = widthSrc  = widthWin  = width;
    heightVid = heightSrc = heightWin = height;

    calculateDestinationArea();
}

void XFVideo::calculateDestinationArea()
{
    topDest = 0;
    leftDest = 0;

    // Keep aspect ratio:
    double ratio = double(widthSrc)/double(heightSrc);
    double iratio = double(heightSrc)/double(widthSrc);
    widthDest = round(ratio * double(heightWin));
    heightDest = round(iratio * double(widthWin));

    if (widthDest<=widthWin)
    {
	heightDest = heightWin;
	leftDest = (widthWin-widthDest)>>1;
    }
    else if (heightDest<=heightWin)
    {
	widthDest = widthWin;
	topDest = (heightWin-heightDest)>>1;
    }
    else
    {
	heightDest = heightWin;
	widthDest = widthWin;
    }

    // Send notification with size information to GUI:
    boost::shared_ptr<NotificationVideoSize> nvs(new NotificationVideoSize());
    nvs->widthVid = widthVid;
    nvs->heightVid = heightVid;
    nvs->widthWin = widthWin;
    nvs->heightWin = heightWin;
    nvs->leftDst = leftDest;
    nvs->topDst = topDest;
    nvs->widthDst = widthDest;
    nvs->heightDst = heightDest;
    nvs->leftSrc = leftSrc;
    nvs->topSrc = topSrc;
    nvs->widthSrc = widthSrc;
    nvs->heightSrc = heightSrc;
    sendNotificationVideoSize(nvs);
}

boost::shared_ptr<XFVideoImage> XFVideo::show(boost::shared_ptr<XFVideoImage> yuvImage)
{    
    boost::shared_ptr<XFVideoImage> previousImage = m_displayedImage;
    m_displayedImage = yuvImage;

    if (useXvClipping)
    {
	show();
    }
    else
    {
	boost::shared_ptr<XFVideoImage> tmp = m_displayedImageClipped;
	m_displayedImageClipped.reset();
	m_displayedImageClipped = boost::make_shared<XFVideoImage>(this, widthSrc,  heightSrc);	

	clip(m_displayedImage, m_displayedImageClipped);

	show();
    }

    return previousImage;
}

void XFVideo::show()
{
    if (m_displayedImage)
    {
	if (useXvClipping)
	{
	    // Display srcArea of yuvImage on destArea of Drawable window:
	    XvShmPutImage(m_display, xvPortId, m_window, gc, m_displayedImage->xvImage(),
			  leftSrc,  topSrc,  widthSrc,  heightSrc,     // srcArea  (x,y,w,h)
			  leftDest, topDest, widthDest, heightDest,    // destArea (x,y,w,h)
			  True);
	}
	else
	{
	    // Display the already clipped xvImage:
	    XvShmPutImage(m_display, xvPortId, m_window, gc, m_displayedImageClipped->xvImage(),
			  0,  0,  widthSrc,  heightSrc,                // srcArea  (x,y,w,h)
			  leftDest, topDest, widthDest, heightDest,    // destArea (x,y,w,h)
			  True);
	}
    
	// Explicitly calling XFlush to ensure that the image is visible:
	XFlush(m_display);
    }
}

void XFVideo::handleConfigureEvent(boost::shared_ptr<WindowConfigureEvent> event)
{
    INFO();
    // This method is called when the widget is resized.
    widthWin  = event->width;
    heightWin = event->height;
    calculateDestinationArea();
}

void XFVideo::handleExposeEvent()
{
    paintBorder();
    show();
}

int XFVideo::xWindow(int xVideo) {return round(leftDest + (double(widthDest) /double(widthSrc))  * (xVideo  - int(leftSrc)));}
int XFVideo::yWindow(int yVideo) {return round(topDest  + (double(heightDest)/double(heightSrc)) * (yVideo  - int(topSrc)));}
int XFVideo::xVideo(int xWindow) {return round(leftSrc  + (double(widthSrc)  /double(widthDest)) * (xWindow - int(leftDest)));}
int XFVideo::yVideo(int yWindow) {return round(topSrc   + (double(heightSrc) /double(heightDest))* (yWindow - int(topDest)));}

typedef int (XFVideo::* Convert_t)(int);

static int calcClip(int origVal, int newVal, XFVideo* obj, Convert_t fct, int defaultVal)
{
    if (newVal == -2) return defaultVal;
    if (newVal == -1) return origVal;
    return (obj->*fct)(newVal);
}

template<typename TVAL>
static void constraint(TVAL& val, int min, int max)
{
    if (val < min) val = min;
    if (val > max) val = max;
}

void XFVideo::clip(int winLeft, int winRight, int winTop, int winButtom)
{
    if (m_displayedImage)
    {
	// std::cout << "win:" << winLeft << ", " << winRight << ", " << winTop << ", " << winButtom << std::endl;

	// Convert window coordinates into video coordinates and handle 
	// special values -1 (keep existing value) and -2 (set back to default).
	int videoLeft   = calcClip(leftSrc,          winLeft,   this, &XFVideo::xVideo, 0);
	int videoRight  = calcClip(leftSrc+widthSrc, winRight,  this, &XFVideo::xVideo, m_displayedImage->width());
	int videoTop    = calcClip(topSrc,           winTop,    this, &XFVideo::yVideo, 0);
	int videoButtom = calcClip(topSrc+heightSrc, winButtom, this, &XFVideo::yVideo, m_displayedImage->height());

	// std::cout << "vid:" << videoLeft << ", " << videoRight << ", " << videoTop << ", " << videoButtom << std::endl;

	// Values may be out of range:
	constraint(videoLeft,   0,           m_displayedImage->width());
	constraint(videoRight,  videoLeft+1, m_displayedImage->width());
	constraint(videoTop,    0,           m_displayedImage->height());
	constraint(videoButtom, videoTop+1,  m_displayedImage->height());

	// Round to muliples of 2, since YUV format is used:
	leftSrc   = 0xfffffffe & (videoLeft);
	widthSrc  = 0xfffffffe & (videoRight  - videoLeft);
	topSrc    = 0xfffffffe & (videoTop);
	heightSrc = 0xfffffffe & (videoButtom - videoTop);

	// std::cout << "src:" << leftSrc << ", " << widthSrc << ", " << topSrc << ", " << heightSrc << std::endl;

	// Update widget:
	calculateDestinationArea();
	paintBorder();
	show(m_displayedImage);
    }
}

void XFVideo::clip(boost::shared_ptr<XFVideoImage> in,
		   boost::shared_ptr<XFVideoImage> out)
{
    XvImage* yuvImageIn  = in->yuvImage;
    XvImage* yuvImageOut = out->yuvImage;

    char* Yin = yuvImageIn->data + yuvImageIn->offsets[0];
    char* Vin = yuvImageIn->data + yuvImageIn->offsets[1];
    char* Uin = yuvImageIn->data + yuvImageIn->offsets[2];

    int pYin = yuvImageIn->pitches[0];
    int pVin = yuvImageIn->pitches[1];
    int pUin = yuvImageIn->pitches[2];

    int wOut = yuvImageOut->width;
    int hOut = yuvImageOut->height;

    char* Yout = yuvImageOut->data + yuvImageOut->offsets[0];
    char* Vout = yuvImageOut->data + yuvImageOut->offsets[1];
    char* Uout = yuvImageOut->data + yuvImageOut->offsets[2];

    int pYout = yuvImageOut->pitches[0];
    int pVout = yuvImageOut->pitches[1];
    int pUout = yuvImageOut->pitches[2];

    for (int y=0; y<hOut; y++)
	memcpy(&Yout[y*pYout], &Yin[leftSrc+ (topSrc+y)*pYin], wOut);

    int wOut2 = wOut/2;
    int hOut2 = hOut/2;
    int leftSrc2 = leftSrc/2;
    int topSrc2  = topSrc/2;
	
    for (int y=0; y<hOut2; y++)
    {
	memcpy(&Uout[y*pUout], &Uin[leftSrc2+ (topSrc2+y)*pUin], wOut2);
	memcpy(&Vout[y*pVout], &Vin[leftSrc2+ (topSrc2+y)*pVin], wOut2);
    }
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
    const unsigned short w3 = widthWin-leftDest-widthDest;

    const unsigned short h1 = topDest;
    const unsigned short h2 = heightDest;
    const unsigned short h3 = heightWin-topDest-heightDest;

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
    init(xfVideo.get(), xfVideo->widthVid, xfVideo->heightVid);
}

XFVideoImage::XFVideoImage(XFVideo* xfVideo, int width, int height)
    : pts(0)
{
    init(xfVideo, width, height);
}

void XFVideoImage::init(XFVideo* xfVideo, int width, int height)
{
    yuvImage = XvShmCreateImage(xfVideo->display(),
				xfVideo->xvPortId,
				xfVideo->imageFormat,
				0, // char* data
				width, height,
				&yuvShmInfo);

    DEBUG(<< std::dec << "requested: " << width << "*" << height
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

    m_display = xfVideo->display();
    // Attach the shared memory to the X server:
    if (!XShmAttach(m_display, &yuvShmInfo))
    {
	THROW(XFException, << "XShmAttach failed !");
    }

    DEBUG( "yuvImage data_size = " << std::dec << yuvImage->data_size );
}

XFVideoImage::~XFVideoImage()
{
    // Detach shared memory from X server:
    XShmDetach(m_display, &yuvShmInfo);

    // Detach shared memory from the address space of the calling process:
    shmdt(shmAddr);

    // Mark the shared memory segment to be destroyed.
    // The invers operation of shmget:
    shmctl(yuvShmInfo.shmid, IPC_RMID, 0);

    XFree(yuvImage);
}

void XFVideoImage::createBlackImage()
{
    int h  = height();
    int h2 = h/2;

    char* Y = data() + yuvImage->offsets[0];
    char* V = data() + yuvImage->offsets[1];
    char* U = data() + yuvImage->offsets[2];

    int pYin = yuvImage->pitches[0];
    int pVin = yuvImage->pitches[1];
    int pUin = yuvImage->pitches[2];

    memset(Y,   0, pYin*h);
    memset(U, 128, pUin*h2);
    memset(V, 128, pVin*h2);
}

void XFVideoImage::createPatternImage()
{
    int w = width();
    int h = height();

    char* Y = data() + yuvImage->offsets[0];
    char* V = data() + yuvImage->offsets[1];
    char* U = data() + yuvImage->offsets[2];

    for (int x=0; x<w; x++)
	for (int y=0; y<h; y++)
	{
	    Y[x + y*yuvImage->pitches[0]] = 0xff & ((x/32 + y/32)*20);
	}

    for (int x=0; x<w/2; x++)
	for (int y=0; y<h/2; y++)
	{
	    U[x + y*yuvImage->pitches[2]] = 0xff & ((x/16 + y/16)*20);
	    V[x + y*yuvImage->pitches[1]] = 0xff & ((x/16 + y/16)*20);
	}
}

void XFVideoImage::createDemoImage()
{
    int w = width();
    int h = height();

    char* Y = data() + yuvImage->offsets[0];
    char* V = data() + yuvImage->offsets[1];
    char* U = data() + yuvImage->offsets[2];

    for (int x=0; x<w; x++)
	for (int y=0; y<h; y++)
	{
	    Y[x + y*yuvImage->pitches[0]] = 128;
	}

    for (int x=0; x<w/2; x++)
	for (int y=0; y<h/2; y++)
	{
	    U[x + y*yuvImage->pitches[2]] = x * 255/ (w/2);
	    V[x + y*yuvImage->pitches[1]] = y * 255/ (h/2);
	}
}

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

// -------------------------------------------------------------------
