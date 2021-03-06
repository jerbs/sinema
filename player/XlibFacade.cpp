//
// X11 and Xv Extension Interface
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

#include "player/XlibFacade.hpp"
#include "player/XlibHelpers.hpp"
#include "platform/Logging.hpp"

#include <sys/ipc.h>  // to allocate shared memory
#include <sys/shm.h>  // to allocate shared memory
#include <errno.h>
#include <string.h>   // memcpy, strerror
#include <math.h>

// #undef TRACE_DEBUG
// #define TRACE_DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

using namespace std;

// -------------------------------------------------------------------

extern int tidVideoOutput;

int segmentationFault()
{
    int* a = 0;
    return *a;
}

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
	    TRACE_DEBUG( << "Found " << visualInfo[i].depth << " bit "
		   << visualInfo[i].className );
	    TRACE_DEBUG( << xVisualInfo );
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

NeedsXShm::NeedsXShm(Display* display)
{
    // Check to see if the shared memory extension is available:
    bool shmExtAvailable = XShmQueryExtension(display);
    if (!shmExtAvailable)
    {
	TRACE_THROW(XFException, << "No shared memory extension available.");
    }
}

// -------------------------------------------------------------------

NeedsXv::NeedsXv(Display* display)
{
    // Check to see if Xv extension is available:
    switch(XvQueryExtension(display,
			    &xvVersionInfo.version,
			    &xvVersionInfo.revision,
			    &xvVersionInfo.request_base,
			    &xvVersionInfo.event_base,
			    &xvVersionInfo.error_base))
    {
    case Success:
	break;
    case XvBadExtension:
    case XvBadAlloc:
    default:
	TRACE_THROW(XFException, << "No Xv extension available.");
    }
}

// -------------------------------------------------------------------

XFVideo::XFVideo(Display* display, Window window,
		 unsigned int width, unsigned int height,
		 send_notification_video_size_fct_t fct,
		 send_notification_clipping_fct_t fct2,
		 send_notification_video_attribute_fct_t fct3,
		 boost::shared_ptr<AddWindowSystemEventFilterFunctor> addWindowSystemEventFilter)
    : NeedsXShm(display),
      NeedsXv(display),
      m_display(display),
      m_window(window),
      xvPortId(INVALID_XV_PORT_ID),
      fourccFormat(0),
      displayedFourccFormat(0),
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
      sendNotificationClipping(fct2),
      sendNotificationVideoAttribute(fct3),
      addWindowSystemEventFilter(addWindowSystemEventFilter)
{
    grabXvPort();

    // Initialize image format:
    fourccFormat = GUID_YUV12_PLANAR;     // Best performance for non-interlaced video.

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

bool XFVideo::grabXvPort(XvPortID adaptorPort)
{
    switch(XvGrabPort( m_display, adaptorPort, CurrentTime ))
    {
    case Success:
	xvPortId = adaptorPort;
	TRACE_DEBUG( << "xvPortId = " << xvPortId);
	fillFourccFormatList();
	selectXvPortNotify();
	notifyXvPortAttributes();
	return true;
    case XvAlreadyGrabbed:
	TRACE_DEBUG( << "XvGrabPort failed: XvAlreadyGrabbed");
	break;
    default:
	TRACE_ERROR( << "XvGrabPort failed for adaptorPort " 
		     << adaptorPort << "." );
    }

    return false;
}

void XFVideo::grabXvPort()
{
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
	TRACE_THROW(XFException, << "XvQueryAdaptors failed.");
    }

    xvPortId = INVALID_XV_PORT_ID;
    fourccFormat = 0;
    
    for (unsigned int i=0; i<num_ai; i++)
    {
	TRACE_DEBUG( << "XvAdaptorInfo[" << i << "]:\n" << ai[i] );

	for (XvPortID adaptorPort = ai[i].base_id;
	     adaptorPort < ai[i].base_id+ai[i].num_ports;
	     adaptorPort++)
	{
	    if (xvPortId == INVALID_XV_PORT_ID)
	    {
		if ((ai[i].type & XvInputMask) &&
		    (ai[i].type & XvImageMask))
		{
		    if (grabXvPort(adaptorPort))
		    {
			dumpXvEncodings(adaptorPort);
			dumpXvAttributes(adaptorPort);
			dumpXvImageFormat(adaptorPort);

			// With the current implementation the Xv port must
			// support YV12 and YUY2. The VideoDecoder selects
			// one of these 2 formats:
			if (isFourccFormatValid(GUID_YUV12_PLANAR) &&
			    isFourccFormatValid(GUID_YUY2_PACKED) )
			{
			    XvFreeAdaptorInfo(ai);
			    return;
			}
		    }
		}
	    }
	}
    }

    XvFreeAdaptorInfo(ai);

    TRACE_THROW(XFException, << "No XvAdaptorPort found."); 
}

void XFVideo::ungrabXvPort()
{
    TRACE_DEBUG();
    fourccFormatList.clear();
    XvUngrabPort(m_display, xvPortId, CurrentTime);
    xvPortId = INVALID_XV_PORT_ID;
}

void XFVideo::regrabXvPort()
{
    // Without this sequence the fglrx closed source driver for ATI/AMD graphic
    // cards fails to display Xv images with different formats. The Y plane is 
    // as expected, but UV is wrong.

    // Using XvGrabPort on an Xv port already grabbed by the application returns
    // SUCCESS. XvAlreadyGrabbed is returned by another application/X11 client
    // only. Since the number of Xv port is very limited, the application should
    // use only one port.

    XvPortID oldXvPortId = xvPortId;
    ungrabXvPort();

    // Try to grab the same Xv port again:
    grabXvPort(oldXvPortId);

    // This may have failed.
    if (xvPortId == INVALID_XV_PORT_ID)
    {
	// Try to grab an available Xv port:
	grabXvPort();

	// This may have failed too, but that's bad luck.
    }
}

void XFVideo::dumpXvEncodings(XvPortID adaptorPort)
{
    // Querry XvEncodingInfo:
    unsigned int num_ei;
    XvEncodingInfo* ei;
    switch(XvQueryEncodings(m_display, adaptorPort,
			    &num_ei, &ei))
    {
    case Success:
	break;
    default:
	TRACE_THROW(XFException, << "XvQueryEncodings failed.");
    }

    for (unsigned int j=0; j<num_ei; j++)
    {
	TRACE_DEBUG( << "XvEncodingInfo[" << j << "]:\n" << ei[j] );
    }

    XvFreeEncodingInfo(ei);
}

void XFVideo::dumpXvAttributes(XvPortID adaptorPort)
{
    // Querry XvAttributes
    int num_att;
    XvAttribute* att = XvQueryPortAttributes(m_display, adaptorPort, &num_att);
    if (att)
    {
	for (int k=0; k<num_att; k++)
	{
	    TRACE_DEBUG( << "XvAttribute[" << k << "]\n" << att[k] );
	    if (att[k].flags & XvGettable)
	    {
		char* atom_name = att[k].name;
		Bool only_if_exists = true;
		Atom attribute = XInternAtom(m_display, atom_name, only_if_exists);

		switch(attribute)
		{
		case None:
		    break;
		default:
		    int value;
		    switch(XvGetPortAttribute(m_display, adaptorPort, attribute, &value))
		    {
		    case Success:
			TRACE_DEBUG( << "XvGetPortAttribute: " << atom_name << " = " << value );
		    case BadAlloc:
		    case BadAtom:
		    case BadValue:
			break;
		    default:
			break;
		    }
		}
	    }
	}
	XFree(att);
    }
}

void XFVideo::dumpXvImageFormat(XvPortID adaptorPort)
{
    // Querry XvImageFormatValues
    int num_ifv;
    XvImageFormatValues* ifv = XvListImageFormats(m_display, adaptorPort, &num_ifv);
    if (ifv)
    {
	for (int l=0; l<num_ifv; l++)
	{
	    TRACE_DEBUG( << "XvImageFormatValues[" << l << "]\n" << ifv[l] );
	}
	XFree(ifv);
    }
}

void XFVideo::fillFourccFormatList()
{
    // This function is called each time a new XvPort is grabed.
    // As a work-around the port is regrabed when the image format changes.
    // The fourccFormatList (theoretically) may change each time this function is called.

    TRACE_DEBUG();
    fourccFormatList.clear();
    int num_ifv;
    XvImageFormatValues* ifv = XvListImageFormats(m_display, xvPortId, &num_ifv);
    if (ifv)
    {
	for (int l=0; l<num_ifv; l++)
	{
	    int fourccFormat = ifv[l].id;
	    TRACE_DEBUG(<< "adding fourcc format id " << std::hex << fourccFormat);
	    fourccFormatList.push_back(fourccFormat);
	}
	XFree(ifv);
    }
}

void XFVideo::notifyXvPortAttributes()
{
    struct XvPortAttribute
    {
	std::string name;
	int value;
	int min_value;
	int max_value;
	bool settable;
	bool gettable;
    };

    std::vector<XvPortAttribute> attributes;

    int num_att;
    XvAttribute* att = XvQueryPortAttributes(m_display, xvPortId, &num_att);
    if (att)
    {
	for (int k=0; k<num_att; k++)
	{
	    if (att[k].flags & XvGettable)
	    {
		char* atom_name = att[k].name;
		Bool only_if_exists = true;
		Atom attribute = XInternAtom(m_display, atom_name, only_if_exists);

		switch(attribute)
		{
		case None:
		    break;
		default:
		    int value;
		    switch(XvGetPortAttribute(m_display, xvPortId, attribute, &value))
		    {
		    case Success:
			{
			    XvPortAttribute xpa;
			    xpa.name = att[k].name;
			    xpa.value = value;
			    xpa.min_value = att[k].min_value;
			    xpa.max_value = att[k].max_value;
			    xpa.settable = att[k].flags & XvSettable;
			    xpa.gettable = att[k].flags & XvGettable;
			    attributes.push_back(xpa);

			    sendNotificationVideoAttribute(boost::make_shared<NotificationVideoAttribute>(att[k].name,
													  value,
													  att[k].min_value,
													  att[k].max_value,
													  att[k].flags & XvSettable));
			}
		    case BadAlloc:
		    case BadAtom:
		    case BadValue:
			break;
		    default:
			break;
		    }
		}
	    }
	}
	XFree(att);
    }
}

void XFVideo::setXvPortAttributes(std::string const& name, int value)
{
    const char* atom_name = name.c_str();
    Bool only_if_exists = true;
    Atom attribute = XInternAtom(m_display, atom_name, only_if_exists);
    XvSetPortAttribute(m_display, xvPortId, attribute, value);
}

class XvWindowSystemEventFilterFunctor : public WindowSystemEventFilterFunctor
{
public:
    XvWindowSystemEventFilterFunctor(XFVideo& xfVideo, XvVersionInfo const& xvVersionInfo)
	: xfVideo(xfVideo),
	  xvVersionInfo(xvVersionInfo)
    {}

    bool operator()(void* gui_event)
    {
	XEvent* xevent = (XEvent*)gui_event;
	if (xevent->type == xvVersionInfo.event_base + XvPortNotify)
	{
	    XvPortNotifyEvent *ev = (XvPortNotifyEvent *) xevent;
	    if (ev->display == xfVideo.m_display &&
		ev->port_id == xfVideo.xvPortId)
	    {
		const char* name = XGetAtomName(ev->display, ev->attribute);
		TRACE_DEBUG(<< "XvPortNotify: "<< name << " = " << ev->value);

		xfVideo.sendNotificationVideoAttribute(boost::make_shared<NotificationVideoAttribute>(name,
												      ev->value));
	    }

	    return true;
	}

	return false;
    }

private:
    XFVideo& xfVideo;
    XvVersionInfo const& xvVersionInfo;
};

void XFVideo::selectXvPortNotify()
{
    if (addWindowSystemEventFilter)
    {
	(*addWindowSystemEventFilter)(boost::make_shared<XvWindowSystemEventFilterFunctor>(*this, xvVersionInfo));
    }

    const Bool enable = true;
    switch(XvSelectPortNotify(m_display, xvPortId, enable))
    {
    case Success:
	break;
    default:
	TRACE_THROW(XFException, << "XvSelectPortNotify failed.");
    }
}

void XFVideo::selectEvents()
{
    // Select events:
    XSelectInput(m_display, m_window, StructureNotifyMask | ExposureMask | KeyPressMask);
}

bool XFVideo::isFourccFormatValid(int fourccFormat)
{
    for (std::vector<int>::iterator it = fourccFormatList.begin(); it != fourccFormatList.end(); it++)
    {
	if (*it == fourccFormat)
	    return true;
    }
    return false;
}

void XFVideo::resize(unsigned int width, unsigned int height,
		     unsigned int parNum, unsigned int parDen,
		     int fourccFormat)
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

    sendNotificationClipping(boost::make_shared<NotificationClipping>
			     (leftSrc, leftSrc+widthSrc, topSrc, topSrc+heightSrc));

    // Sample aspect ratio:
    this->parNum = parNum;
    this->parDen = parDen;

    calculateDestinationArea(NotificationVideoSize::VideoSizeChanged);

    // Video format:
    this->fourccFormat = fourccFormat;

    if (!isFourccFormatValid(fourccFormat))
    {
	TRACE_THROW(std::string, << "Unsupported fourcc format: " << std::hex << fourccFormat);
    }
}

void XFVideo::calculateDestinationArea(NotificationVideoSize::Reason reason)
{
    topDest = 0;
    leftDest = 0;

    // pixel aspect ratio:
    double par = double(parNum)/double(parDen);

    TRACE_DEBUG( << "reason = " << reason << ", par = " << par);

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

std::unique_ptr<XFVideoImage> XFVideo::show(std::unique_ptr<XFVideoImage> yuvImage)
{    
    std::unique_ptr<XFVideoImage> previousImage = std::move(m_displayedImage);
    m_displayedImage = std::move(yuvImage);

    if (m_noClippingNeeded || m_useXvClipping)
    {
	show();
    }
    else
    {
	std::unique_ptr<XFVideoImage> tmp = std::move(m_displayedImageClipped);
	m_displayedImageClipped.reset();
	// FIXME: Do not allocate a new image each time:
	m_displayedImageClipped = std::unique_ptr<XFVideoImage>(new XFVideoImage(this, widthSrc,  heightSrc,
										 m_displayedImage->yuvImage->id));

	clip(m_displayedImage->yuvImage,
	     m_displayedImageClipped->yuvImage);

	show();
    }

    return previousImage;
}

void XFVideo::show()
{
    if (m_displayedImage)
    {
	int id = m_displayedImage->yuvImage->id;

	TRACE_DEBUG(<< std::hex << id);

	if (displayedFourccFormat != id)
	{
	    displayedFourccFormat = id;
	    regrabXvPort();
	}

        if (m_noClippingNeeded || m_useXvClipping)
	{
#if 0
	    TRACE_DEBUG(<< m_display << ", " << xvPortId << ", " << m_window << ", " 
			<< gc << ", " << *(m_displayedImage->xvImage()) << ", ("
			<< leftSrc << ", " <<  topSrc << ", " <<  widthSrc << ", " <<  heightSrc << ")("
			<< leftDest << ", " << topDest << ", " << widthDest << ", " << heightDest << ")");
#endif

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
	show(std::move(m_displayedImage));  // Looks a bit unclean, here m_displayedImage is updated with itself.
    }
}

void XFVideo::clip(XvImage* yuvImageIn,
		   XvImage* yuvImageOut)
{
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
	TRACE_THROW(std::string, << "unsupported format 0x" << std::hex << yuvImageIn->id);
    }
}

void XFVideo::enableXvClipping()
{
    TRACE_DEBUG();
    m_useXvClipping = true;
}

void XFVideo::disableXvClipping()
{
    TRACE_DEBUG();
    m_useXvClipping = false;
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
    TRACE_DEBUG(<< "tid = " << gettid());
    init(xfVideo.get(), xfVideo->widthVid, xfVideo->heightVid, xfVideo->fourccFormat);
}

XFVideoImage::XFVideoImage(XFVideo* xfVideo, int width, int height, int fourccFormat)
    : pts(0)
{
    TRACE_DEBUG(<< "tid = " << gettid());
    init(xfVideo, width, height, fourccFormat);
}

void XFVideoImage::init(XFVideo* xfVideo, int width, int height, int fourccFormat)
{
    if (!xfVideo->isFourccFormatValid(fourccFormat))
    {
	TRACE_THROW(std::string, << "No valid fourcc format id set.");
    }

    m_requestedWidth = width;
    m_requestedHeight = height;

    yuvImage = XvShmCreateImage(xfVideo->display(),
				xfVideo->xvPortId,
				fourccFormat,
				0, // char* data
				width, height,
				&yuvShmInfo);

    if (!yuvImage)
    {
	TRACE_THROW(std::string, << "XvShmCreateImage failed.");
    }

    TRACE_DEBUG(<< std::dec << "requested: " << width << "*" << height
		<< " got: " << yuvImage->width << "*" << yuvImage->height);
    TRACE_DEBUG(<< "yuvImage = " << std::hex << uint64_t(yuvImage));
    TRACE_DEBUG(<< "fourccFormat  = " << std::hex << fourccFormat);
    TRACE_DEBUG(<< "yuvImage->id = " << std::hex << yuvImage->id);
    for (int i=0; i<yuvImage->num_planes; i++)
    {
	TRACE_DEBUG(<< "yuvImage->offsets[" << std::dec << i << "] = " << yuvImage->offsets[i]);
    }

    // Allocate the shared memory requested by the server:
    yuvShmInfo.shmid = shmget(IPC_PRIVATE,
			      yuvImage->data_size,
			      IPC_CREAT | 0777);
    if (yuvShmInfo.shmid == -1)
    {
	TRACE_THROW(XFException,
		    << "shmget failed"
		    << " data_size=" << yuvImage->data_size
		    << " errno=" << strerror(errno) << "(" << errno << ")");
    }

    // Attach the shared memory to the address space of the calling process:
    shmAddr = shmat(yuvShmInfo.shmid, 0, 0);
    if (shmAddr == (void*)-1)
    {
	TRACE_THROW(XFException,
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
	TRACE_THROW(XFException, << "XShmAttach failed !");
    }

    TRACE_DEBUG( "yuvImage data_size = " << std::dec << yuvImage->data_size );
}

XFVideoImage::~XFVideoImage()
{
    TRACE_DEBUG(<< "tid = " << gettid());

    if (gettid() != tidVideoOutput)
    {
	TRACE_DEBUG(<< "tid = " << gettid() << ", Upps");
	segmentationFault();
    }

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
	TRACE_THROW(std::string, << "unsupported format 0x" << std::hex << yuvImage->id);
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
	TRACE_THROW(std::string, << "unsupported format 0x" << std::hex << yuvImage->id);
    }
}

void XFVideoImage::createDemoImage()
{
    int w = width();
    int h = height();
    std::cout << "size = " << std::dec << w << ", " << h << ", 0x" << std::hex << yuvImage->id << std::dec << std::endl;

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
	TRACE_THROW(std::string, << "unsupported format 0x" << std::hex << yuvImage->id);
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

DeleteXFVideoImage::DeleteXFVideoImage(std::unique_ptr<XFVideoImage>&& image)
    : image(std::move(image))
{
    TRACE_DEBUG(<< "tid = " << gettid());
}

DeleteXFVideoImage::~DeleteXFVideoImage()
{
    TRACE_DEBUG(<< "tid = " << gettid());
}

// -------------------------------------------------------------------

std::ostream& operator<<(std::ostream& strm, const NotificationVideoSize& nvs)
{
    strm << std::dec << nvs.reason << ","
	 << "vid(" << nvs.widthVid << "," << nvs.heightVid << ")"
	 << "win(" << nvs.widthWin << "," << nvs.heightWin << ")"
	 << "dst(" << nvs.widthDst << "," << nvs.heightDst << ")"
	 << "src(" << nvs.widthSrc << "," << nvs.heightSrc << ")"
	 << "adj(" << nvs.widthAdj << "," << nvs.heightAdj << ")";

    return strm;
}

std::ostream& operator<<(std::ostream& strm, const NotificationVideoSize::Reason& reason)
{
    switch (reason)
    {
    case NotificationVideoSize::VideoSizeChanged:
	strm << "VidSizeChanged";
	break;
    case NotificationVideoSize::WindowSizeChanged:
	strm << "WinSizeChanged";
	break;
    case NotificationVideoSize::ClippingChanged:
	strm << "ClippingModified";
	break;
    default:
	strm << int(reason);
	break;
    }

    return strm;
}
