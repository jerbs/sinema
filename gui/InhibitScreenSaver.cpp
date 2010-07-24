//
// Inhibit Screen Saver
//
// Copyright (C) Joachim Erbs, 2010
//

#include "InhibitScreenSaver.hpp"
#include "platform/Logging.hpp"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

// #undef TRACE_DEBUG 
// #define TRACE_DEBUG(text) std::cout << __PRETTY_FUNCTION__ text << std::endl;

// -------------------------------------------------------------------

class XScreenSaverInterface
{
 public:
    XScreenSaverInterface(Display* display);
    ~XScreenSaverInterface();

    void simulateUserActivity();

 private:
    void findScreenSaverWindow();

    
    Display* m_xdisplay;
    Window m_ScreenSaverWindow;

    static int errorHandler(Display *dpy, XErrorEvent *error);

    static XErrorHandler oldErrorHandler;
    static bool gotBadWindow;
};

XErrorHandler XScreenSaverInterface::oldErrorHandler = 0;
bool XScreenSaverInterface::gotBadWindow;

// -------------------------------------------------------------------

InhibitScreenSaver::InhibitScreenSaver()
{
}

extern Display * get_x_display(Gtk::Widget & widget);

void InhibitScreenSaver::on_realize(Gtk::Widget* widget)
{
    m_XScreenSaverInterface = boost::make_shared<XScreenSaverInterface>(get_x_display(*widget));
}

void InhibitScreenSaver::simulateUserActivity()
{
    if (m_XScreenSaverInterface)
	m_XScreenSaverInterface->simulateUserActivity();
}

// -------------------------------------------------------------------

XScreenSaverInterface::XScreenSaverInterface(Display* display)
    : m_xdisplay(display)
{
    oldErrorHandler = XSetErrorHandler (errorHandler);
    findScreenSaverWindow();
}

XScreenSaverInterface::~XScreenSaverInterface()
{
    XSetErrorHandler (oldErrorHandler);
}

int XScreenSaverInterface::errorHandler(Display *dpy, XErrorEvent *error)
{
    if (error->error_code == BadWindow)
    {
	// Catch BadWindow error.
	gotBadWindow = true;
	return 0;
    }

    if (oldErrorHandler)
	return (*oldErrorHandler) (dpy, error);

    return 0;
}

void XScreenSaverInterface::findScreenSaverWindow()
{
    Window window = RootWindowOfScreen (DefaultScreenOfDisplay (m_xdisplay));
    Window root;
    Window parent;
    Window *children = 0;
    unsigned int nchildren;

    if (! XQueryTree (m_xdisplay, window, &root, &parent, &children, &nchildren))
	return;

    if (children)
    {
	if (window != root)
	    return;

	if (parent)
	    return;

	for (unsigned int i = 0; i < nchildren; i++)
	{
	    XSync (m_xdisplay, False);

	    gotBadWindow = False;

	    Atom XA_SCREENSAVER_VERSION = XInternAtom (m_xdisplay, "_SCREENSAVER_VERSION",False);
	    Atom actual_type;
	    int actual_format;
	    unsigned long nitems;
	    unsigned long bytes_after;
	    unsigned char *prop;

	    int status = XGetWindowProperty (m_xdisplay, children[i], XA_SCREENSAVER_VERSION,
					     0, 200, False, XA_STRING, &actual_type,
					     &actual_format, &nitems, &bytes_after, &prop);

	    XSync(m_xdisplay, False);

	    if (gotBadWindow)
	    {
		status = BadWindow;
		gotBadWindow = false;
	    }

	    if (status == Success && actual_type != None)
	    {
		m_ScreenSaverWindow = children[i];
	    }
	}

	XFree (children);
    }
}

void XScreenSaverInterface::simulateUserActivity()
{
    if (!m_ScreenSaverWindow)
	return;

    if (gotBadWindow)
	findScreenSaverWindow();

    gotBadWindow = False;

    Atom XA_SCREENSAVER = XInternAtom (m_xdisplay, "SCREENSAVER", False);
    Atom XA_DEACTIVATE = XInternAtom(m_xdisplay, "DEACTIVATE", False);

    XEvent event;
    event.xany.type = ClientMessage;
    event.xclient.display = m_xdisplay;
    event.xclient.window = m_ScreenSaverWindow;
    event.xclient.message_type = XA_SCREENSAVER;
    event.xclient.format = 32;
    memset (&event.xclient.data, 0, sizeof(event.xclient.data));
    event.xclient.data.l[0] = (long)XA_DEACTIVATE;
    event.xclient.data.l[1] = 0;
    event.xclient.data.l[2] = 0;

    if (! XSendEvent (m_xdisplay, m_ScreenSaverWindow, False, 0L, &event))
    {
	TRACE_ERROR(<< "XSendEvent failed");
    }

    XFlush(m_xdisplay);
}

// -------------------------------------------------------------------
