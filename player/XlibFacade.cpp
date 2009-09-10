#include "XlibFacade.hpp"
#include "XlibHelpers.hpp"
#include "Logging.hpp"

#include <sys/ipc.h>  // to allocate shared memory
#include <sys/shm.h>  // to allocate shared memory

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
	// Wait until a requested event occurs (XClush is called by XNextEvent):
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

XFVideo::XFVideo(unsigned int width, unsigned int height)
    : m_xfWindow(new XFWindow(width, height)),
      m_display(m_xfWindow->display()),
      m_window(m_xfWindow->window()),
      yuvWidth(width),
      yuvHeight(height),
      ratio(double(width)/double(height)),
      iratio(double(height)/double(width))
{
    // Check to see if the shared memory extension is available:
    bool shmExtAvailable = XShmQueryExtension(m_display);
    if (!shmExtAvailable)
    {
	throw XFException("No shared memory extension available.");
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
	throw XFException("No Xv extension available.");
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
	throw XFException("XvQueryAdaptors failed.");
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
		throw XFException("XvQueryEncodings failed.");
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
	throw XFException("No XvAdaptorPort found.");
    }

    if (!imageFormat)
    {
	throw XFException("Needed image format not found.");
    }

    // Create a graphics context
    gc = XCreateGC(display(), window(), 0, 0);

    calculateDestinationArea();
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
    unsigned int width, height;
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
    bool update = true;

    while (update)
    {
	update = false;

	// Display srcArea of yuvImage on destArea of Drawable window:
	XvShmPutImage(m_display, xvPortId, m_window, gc, yuvImage->xvImage(),
		      0, 0, yuvImage->width(), yuvImage->height(), // srcArea  (x,y,w,h)
		      leftDest, topDest, widthDest, heightDest,    // destArea (x,y,w,h)
		      True);

	// Explicitly calling XFlush to ensure that the image is visible:
	XFlush(m_display);

	XEvent xevent;
	while (XCheckMaskEvent(m_display,
			       KeyPressMask | ExposureMask | StructureNotifyMask,
			       &xevent))
	{
	    // See 'man XEvent' for a list of events.
	    // Wait until an event occurs is possible with:
	    // XNextEvent( m_display, &xevent );

	    DEBUG(<< "xevent.type = " << xevent.type);

	    switch ( xevent.type )
	    {
	    case MapNotify:
	    case ConfigureNotify:
		calculateDestinationArea();
		break;
	    case Expose:
		// Window is damaged. X server may send several Expose events.
		if (xevent.xexpose.count == 0)  // The last Expose event.
		    update = true;
		break;
	    case KeyPress:
		DEBUG( << "KeyPress = " << xevent.xkey.keycode );
		// exit(0);
		if ( xevent.xkey.keycode == 0x09 )
		    // quit = true;   FIXME
		    break;
	    default:
		break;
	    }
	}
    }
 
    boost::shared_ptr<XFVideoImage> previousImage = m_displayedImage;
    m_displayedImage = yuvImage;

    return previousImage;
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

    // Allocate the shared memory requested by the server:
    yuvShmInfo.shmid = shmget(IPC_PRIVATE,
			      yuvImage->data_size,
			      IPC_CREAT | 0777);
    // Attach the shared memory to the address space of the calling process:
    shmAddr = shmat(yuvShmInfo.shmid, 0, 0);
    if (shmAddr == (void*)-1)
    {
	throw XFException("shmat failed");
    }

    // Update XShmSegmentInfo and XvImage with pointer to shared memory:
    yuvImage->data = (char*)shmAddr;
    yuvShmInfo.shmaddr = (char*)shmAddr;
    yuvShmInfo.readOnly = False;
  
    // Attach the shared memory to the X server:
    if (!XShmAttach(xfVideo->display(), &yuvShmInfo))
    {
	throw XFException("XShmAttach failed !");
    }

    DEBUG( "yuvImage data_size = " << std::dec << yuvImage->data_size );
}

XFVideoImage::~XFVideoImage()
{
    XFree(yuvImage);
    // Detach shared memory from the address space of the calling process:
    shmdt(shmAddr);
    // Invers operation of shmget?
}

// -------------------------------------------------------------------
