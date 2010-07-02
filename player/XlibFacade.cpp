//
// X11 and Xv Extension Interface
//
// Copyright (C) Joachim Erbs, 2009-2010
//

#include "player/XlibFacade.hpp"
#include "player/XlibHelpers.hpp"
#include "platform/Logging.hpp"

#include <sys/ipc.h>  // to allocate shared memory
#include <sys/shm.h>  // to allocate shared memory
#include <errno.h>
#include <string.h>   // memcpy, strerror
#include <math.h>

// #undef DEBUG
// #define DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

#undef INFO
#define INFO(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

using namespace std;

// -------------------------------------------------------------------

XFDisplay::XFDisplay()
{
    m_display = XOpenDisplay( NULL );
    if (!m_display)
    {
	throw XFException("Cannot open Display");
    }
}

XFDisplay::~XFDisplay()
{
    XCloseDisplay( m_display );
}

// -------------------------------------------------------------------

XFWindow::XFWindow(unsigned int width, unsigned int height)
    : m_xfDisplay(new XFDisplay()),
      m_display(m_xfDisplay->display()),
      m_window(0),
      m_width(width),
      m_height(height)
{
    int screen = DefaultScreen(m_display);

    struct VisualInfo
    {
	int depth;
	int c_class;
	const char* className;
    };

    XVisualInfo xVisualInfo;

    VisualInfo visualInfo[] = {
	{24, TrueColor,  "TrueColor"},
	{16, TrueColor,  "TrueColor"},
	{15, TrueColor,  "TrueColor"},
	{ 8, PseudoColor,"PseudoColor"},
	{ 8, GrayScale,  "GrayScale"},
	{ 8, StaticGray, "StaticGray"},
	{ 1, StaticGray, "StaticGray"}
    };

    const int numVisualInfo = sizeof(visualInfo)/sizeof(VisualInfo);
    for (int i=0; i<numVisualInfo; i++)
    {
	if (XMatchVisualInfo(display(),
			     screen,
			     visualInfo[i].depth,
			     visualInfo[i].c_class,
			     &xVisualInfo))
	{
	    DEBUG( << "Found " << visualInfo[i].depth << " bit "
		   << visualInfo[i].className );
	    DEBUG( << xVisualInfo );
	    break;
	}

	if (i == numVisualInfo)
	{
	    throw(XFException("XMatchVisualInfo: Nothing found"));
	}
    }

    Colormap colormap = XCreateColormap(m_display,
					DefaultRootWindow(m_display),
					xVisualInfo.visual,
					AllocNone);

    XSetWindowAttributes xswa;
    xswa.colormap = colormap;
    xswa.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
    xswa.background_pixel = 0;
    xswa.border_pixel = 0;

    // mask specifies attributes defined in xswa:
    unsigned long mask = CWColormap | CWEventMask | CWBackPixel | CWBorderPixel;

    m_window = XCreateWindow(m_display,
			     DefaultRootWindow(m_display),
			     0, 0,        // x,y position
			     m_width,
			     m_height,
			     0,           // Border width
			     xVisualInfo.depth,
			     InputOutput,
			     xVisualInfo.visual,
			     mask, &xswa);

    // Set window title used by window manager:
    XStoreName(m_display, m_window, "Xv Demo Application");

    // Set title used for icon:
    XSetIconName(m_display, m_window, "Xv Demo");

    // Request events from server (This overwrites the event mask):
    XSelectInput(m_display, m_window, StructureNotifyMask);

    // Requst the server to show the window:
    XMapWindow(m_display, m_window);

    // Wait until the server has processed the request:
    XEvent evt;
    do
    {
	// Wait until a requested event occurs (XFlush is called by XNextEvent):
	XNextEvent( m_display, &evt );
    }
    // Wait until the XMapWindow request is processed by the X server.
    while( evt.type != MapNotify );

    // Now the window is visible on the screen.

    // Setting previous event mask again:
    XSelectInput(m_display, m_window, xswa.event_mask);
}

XFWindow::~XFWindow()
{
    XDestroyWindow(m_display, m_window);
}

// -------------------------------------------------------------------

XFVideo::XFVideo(Display* display, Window window,
		 unsigned int width, unsigned int height,
		 send_notification_video_size_fct_t fct,
		 send_notification_clipping_fct_t fct2)
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
      parNum(16),
      parDen(15),
      m_noClippingNeeded(true),
      m_useXvClipping(true),
      sendNotificationVideoSize(fct),
      sendNotificationClipping(fct2)
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
		}
		XFree(ifv);
	    }

	}

	if (xvPortId != INVALID_XV_PORT_ID)
	{
	    int num_ifv;
	    XvImageFormatValues* ifv = XvListImageFormats(m_display, xvPortId, &num_ifv);
	    if (ifv)
	    {
		for (int l=0; l<num_ifv; l++)
		{
		    DEBUG(<< "adding image format id " << std::hex << ifv[l].id)
		    imageFormatList.push_back(ifv[l].id);
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

    // Initialize image format:
    imageFormat = GUID_YUV12_PLANAR;     // Best performance for non-interlaced video.
    if (!isImageFormatValid(imageFormat))
    {
	imageFormat = GUID_YUY2_PACKED;  // Works with interlaced and non-interlaced video.
	if (!isImageFormatValid(imageFormat))
	{
	    THROW(std::string, << "None of the supported video formats is implemented.");
	}
    }

    // Create a graphics context
    gc = XCreateGC(display, window, 0, 0);

    calculateDestinationArea(NotificationVideoSize::VideoSizeChanged);
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

bool XFVideo::isImageFormatValid(int imageFormat)
{
    for (std::list<int>::iterator it = imageFormatList.begin(); it != imageFormatList.end(); it++)
    {
	if (*it == imageFormat)
	    return true;
    }
    return false;
}

void XFVideo::resize(unsigned int width, unsigned int height,
		     unsigned int parNum, unsigned int parDen,
		     int imageFormat)
{
    // This method is called when the video size changes.
    // It is not called when the widget is resized.

    // The new video size:
    widthVid  = width;
    heightVid = height;

    // Display the full image:
    leftSrc = 0;
    topSrc = 0;
    widthSrc  = width;
    heightSrc = height;
    m_noClippingNeeded = true;

    // Sample aspect ratio:
    this->parNum = parNum;
    this->parDen = parDen;

    calculateDestinationArea(NotificationVideoSize::VideoSizeChanged);

    // Video format:
    this->imageFormat = imageFormat;

    if (!isImageFormatValid(imageFormat))
    {
	THROW(std::string, << "Unsupported format id: " << std::hex << imageFormat);
    }
}

void XFVideo::calculateDestinationArea(NotificationVideoSize::Reason reason)
{
    topDest = 0;
    leftDest = 0;

    // pixel aspect ratio:
    double par = double(parNum)/double(parDen);

    DEBUG( << "par = " << par);

    // Keep aspect ratio:
    double ratio = par*double(widthSrc)/double(heightSrc);
    widthDest = round(ratio * double(heightWin));
    heightDest = round(double(widthWin) / ratio);

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
    nvs->reason = reason;
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
    nvs->widthAdj = widthSrc * par;
    nvs->heightAdj = heightSrc;
    sendNotificationVideoSize(nvs);
}

boost::shared_ptr<XFVideoImage> XFVideo::show(boost::shared_ptr<XFVideoImage> yuvImage)
{    
    boost::shared_ptr<XFVideoImage> previousImage = m_displayedImage;
    m_displayedImage = yuvImage;

    if (m_noClippingNeeded || m_useXvClipping)
    {
	show();
    }
    else
    {
	boost::shared_ptr<XFVideoImage> tmp = m_displayedImageClipped;
	m_displayedImageClipped.reset();
	// FIXME: Do not allocate a new image each time:
	m_displayedImageClipped = boost::make_shared<XFVideoImage>(this, widthSrc,  heightSrc,
								   m_displayedImage->yuvImage->id);

	clip(m_displayedImage, m_displayedImageClipped);

	show();
    }

    return previousImage;
}

void XFVideo::show()
{
    DEBUG(<< std::hex << m_displayedImage->yuvImage->id);

    if (m_displayedImage)
    {
	if (m_noClippingNeeded || m_useXvClipping)
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
    // This method is called when the widget is resized.
    widthWin  = event->width;
    heightWin = event->height;
    calculateDestinationArea(NotificationVideoSize::WindowSizeChanged);
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

static int calcClip(int currentVideoPixelVal,
		    int newWidgetPixelVal,
		    XFVideo* obj, Convert_t fct,
		    int defaultVideoPixelVal)
{
    if (newWidgetPixelVal == -2) return defaultVideoPixelVal;
    if (newWidgetPixelVal == -1) return currentVideoPixelVal;
    return (obj->*fct)(newWidgetPixelVal);
}

template<typename TVAL>
static void constraint(TVAL& val, int min, int max)
{
    if (val < min) val = min;
    if (val > max) val = max;
}

void XFVideo::clipDst(int windowLeft, int windowRight, int windowTop, int windowBottom)
{
    // Parameters are widget pixel.

    if (m_displayedImage)
    {
	// std::cout << "win:" << winLeft << ", " << winRight << ", " << winTop << ", " << winBottom << std::endl;

	// Convert window coordinates into video coordinates and handle 
	// special values -1 (keep existing value) and -2 (set back to default).
	int videoLeft   = calcClip(leftSrc,          windowLeft,   this, &XFVideo::xVideo, 0);
	int videoRight  = calcClip(leftSrc+widthSrc, windowRight,  this, &XFVideo::xVideo, m_displayedImage->width());
	int videoTop    = calcClip(topSrc,           windowTop,    this, &XFVideo::yVideo, 0);
	int videoBottom = calcClip(topSrc+heightSrc, windowBottom, this, &XFVideo::yVideo, m_displayedImage->height());

	clipSrc(videoLeft, videoRight, videoTop, videoBottom);
    }
}

void XFVideo::clipSrc(int videoLeft, int videoRight, int videoTop, int videoBottom)
{
    // Parameters are video pixel.

    if (m_displayedImage)
    {
	// std::cout << "vid:" << videoLeft << ", " << videoRight << ", " << videoTop << ", " << videoBottom << std::endl;

	// Values may be out of range:
	constraint(videoLeft,   0,           m_displayedImage->width());
	constraint(videoRight,  videoLeft+1, m_displayedImage->width());
	constraint(videoTop,    0,           m_displayedImage->height());
	constraint(videoBottom, videoTop+1,  m_displayedImage->height());

	// Round to muliples of 2, since YUV format is used:
	leftSrc   = 0xfffffffe & (videoLeft);
	widthSrc  = 0xfffffffe & (videoRight  - videoLeft);
	topSrc    = 0xfffffffe & (videoTop);
	heightSrc = 0xfffffffe & (videoBottom - videoTop);
	
	if (leftSrc == 0 &&
	    widthSrc == widthVid &&
	    topSrc == 0 &&
	    heightSrc == heightVid)
	{
	    m_noClippingNeeded = true;
	}
	else
	{
	    m_noClippingNeeded = false;
	}

	sendNotificationClipping(boost::make_shared<NotificationClipping>
				 (leftSrc, leftSrc+widthSrc, topSrc, topSrc+heightSrc));

	// std::cout << "src:" << leftSrc << ", " << widthSrc << ", " << topSrc << ", " << heightSrc << std::endl;

	// Update widget:
	calculateDestinationArea(NotificationVideoSize::ClippingChanged);
	paintBorder();
	show(m_displayedImage);
    }
}

void XFVideo::clip(boost::shared_ptr<XFVideoImage> in,
		   boost::shared_ptr<XFVideoImage> out)
{
    XvImage* yuvImageIn  = in->yuvImage;
    XvImage* yuvImageOut = out->yuvImage;

    if (yuvImageIn->id == GUID_YUV12_PLANAR)
    {
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
    else if (yuvImageIn->id == GUID_YUY2_PACKED)
    {
	char* Pin  = yuvImageIn->data + yuvImageIn->offsets[0];
	int  pPin  = yuvImageIn->pitches[0];

	char* Pout = yuvImageOut->data + yuvImageOut->offsets[0];
	int  pPout = yuvImageOut->pitches[0];

	int wOut2 = yuvImageOut->width * 2;
	int hOut  = yuvImageOut->height;

	for (int y=0; y<hOut; y++)
	    memcpy(&Pout[y*pPout], &Pin[2*leftSrc+ (topSrc+y)*pPin], wOut2);
    }
    else
    {
	THROW(std::string, << "unsupported format 0x" << std::hex << yuvImageIn->id);
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
    init(xfVideo.get(), xfVideo->widthVid, xfVideo->heightVid, xfVideo->imageFormat);
}

XFVideoImage::XFVideoImage(XFVideo* xfVideo, int width, int height, int imageFormat)
    : pts(0)
{
    init(xfVideo, width, height, imageFormat);
}

void XFVideoImage::init(XFVideo* xfVideo, int width, int height, int imageFormat)
{
    if (imageFormat == 0)
    {
	THROW(std::string, << "No valid format id set.");
    }

    yuvImage = XvShmCreateImage(xfVideo->display(),
				xfVideo->xvPortId,
				imageFormat,
				0, // char* data
				width, height,
				&yuvShmInfo);

    DEBUG(<< std::dec << "requested: " << width << "*" << height
	  << " got: " << yuvImage->width << "*" << yuvImage->height);
    DEBUG(<< "yuvImage = " << std::hex << uint64_t(yuvImage));
    DEBUG(<< "imageFormat  = " << std::hex << imageFormat);
    DEBUG(<< "yuvImage->id = " << std::hex << yuvImage->id);
    for (int i=0; i<yuvImage->num_planes; i++)
    {
	DEBUG(<< "yuvImage->offsets[" << std::dec << i << "] = " << yuvImage->offsets[i]);
    }

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
    if (yuvImage->id == GUID_YUV12_PLANAR)
    {
	int h  = height();

	char* Y = data() + yuvImage->offsets[0];
	char* V = data() + yuvImage->offsets[1];
	char* U = data() + yuvImage->offsets[2];

	int pYin = yuvImage->pitches[0];
	int pVin = yuvImage->pitches[1];
	int pUin = yuvImage->pitches[2];

	memset(Y,   0, pYin*h);
	memset(U, 128, pUin*h/2);
	memset(V, 128, pVin*h/2);
    }
    else if (yuvImage->id == GUID_YUY2_PACKED)
    {
	int w = width();
	int h = height();

	char* Packed = data() + yuvImage->offsets[0];

	for (int x=0; x<w; x++)
	    for (int y=0; y<h; y++)
	    {
		Packed[2*x   + y*yuvImage->pitches[0]] = 0;    // Y
		Packed[2*x+1 + y*yuvImage->pitches[0]] = 0x80; // U,V
	    }
    }
    else
    {
	THROW(std::string, << "unsupported format 0x" << std::hex << yuvImage->id);
    }
}

void XFVideoImage::createPatternImage()
{
    int w = width();
    int h = height();

    if (yuvImage->id == GUID_YUV12_PLANAR)
    {
	char* Y = data() + yuvImage->offsets[0];
	char* V = data() + yuvImage->offsets[1];
	char* U = data() + yuvImage->offsets[2];

	for (int x=0; x<w; x++)
	    for (int y=0; y<h; y++)
	    {
		Y[x + y*yuvImage->pitches[0]] = 0xff & (((x/32 + y/32) % 8) *20);
	    }

	for (int x=0; x<w/2; x++)
	    for (int y=0; y<h/2; y++)
	    {
		U[x + y*yuvImage->pitches[2]] = 0xff & (0x80 +((x/16 - y/16) % 8) *10);
		V[x + y*yuvImage->pitches[1]] = 0xff & (0x80 -((x/16 + y/16) % 8) *10);
	    }
    }
    else if (yuvImage->id == GUID_YUY2_PACKED)
    {
	char* Packed = data() + yuvImage->offsets[0];

	for (int x=0; x<w; x++)
	    for (int y=0; y<h; y++)
	    {
		Packed[2*x + y*yuvImage->pitches[0]] = 0xff & (((x/32 + y/32) % 8) *20);
	    }

	for (int x=0; x<w/2; x++)
	    for (int y=0; y<h; y++)
	    {
		Packed[4*x+1 + y*yuvImage->pitches[0]] = 0xff & (0x80 +((x/16 - y/32) % 8) *10);
		Packed[4*x+3 + y*yuvImage->pitches[0]] = 0xff & (0x80 -((x/16 + y/32) % 8) *10);
	    }
    }
    else
    {
	THROW(std::string, << "unsupported format 0x" << std::hex << yuvImage->id);
    }
}

void XFVideoImage::createDemoImage()
{
    int w = width();
    int h = height();
    std::cout << "size = " << std::dec << w << ", " << h << ", 0x" << std::hex << yuvImage->id << std::endl;

    if (yuvImage->id == GUID_YUV12_PLANAR)
    {
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
    else if (yuvImage->id == GUID_YUY2_PACKED)
    {
	char* Packed = data() + yuvImage->offsets[0];

	for (int x=0; x<w; x++)
	    for (int y=0; y<h; y++)
	    {
		Packed[2*x + y*yuvImage->pitches[0]] = 128;  // Y
	    }

	for (int x=0; x<w/2; x++)
	    for (int y=0; y<h; y++)
	    {
		Packed[4*x+1 + y*yuvImage->pitches[0]] = x * 255/ (w/2); // U
		Packed[4*x+3 + y*yuvImage->pitches[0]] = y * 255/ (h);   // V
	    }
    }
    else
    {
	THROW(std::string, << "unsupported format 0x" << std::hex << yuvImage->id);
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
