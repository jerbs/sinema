//
// Inhibit Screen Saver
//
// Copyright (C) Joachim Erbs
//

#include "InhibitScreenSaver.hpp"
#include "platform/Logging.hpp"

#include <dbus-cxx.h>
#include <dbus-cxx-glibmm/dispatcher.h>

#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

// #undef DEBUG 
// #define DEBUG(text) std::cout << __PRETTY_FUNCTION__ text << std::endl;

// -------------------------------------------------------------------

class InhibitScreenSaverImpl
{
    friend class InhibitScreenSaver;

    InhibitScreenSaverImpl();

    void connect();

    void simulateUserActivity();

    void sendGetOwnerNameRequest();
    void recvGetNameOwnerResponse();

    void sendSimulateUserActivityRequest();
    void recvSimulateUserActivityResponse();

    DBus::Glib::Dispatcher::pointer m_dispatcher;
    DBus::Connection::pointer m_connection;
    DBus::ObjectProxy::pointer m_object_DBus;
    DBus::ObjectProxy::pointer m_object_ScreenSaver;
    DBus::PendingCall::pointer m_pending;

    DBus::MethodProxyBase::pointer m_method_DBus_GetNameOwner;
    DBus::MethodProxyBase::pointer m_method_ScreenSaver_SimulateUserActivity;

    struct DBusObject
    {
	const char* busName;
	const char* path;
    };

    static const DBusObject screenSaver[];
    static const int numScreenSaver;

    int m_index;
    bool m_connected;
};

const InhibitScreenSaverImpl::DBusObject InhibitScreenSaverImpl::screenSaver[] = {
    {"org.freedesktop.ScreenSaver", "/ScreenSaver"},
    {"org.gnome.ScreenSaver", "/"}
};

const int InhibitScreenSaverImpl::numScreenSaver =
    sizeof(InhibitScreenSaverImpl::screenSaver)/sizeof(InhibitScreenSaverImpl::DBusObject);

// -------------------------------------------------------------------

class XScreenSaverInterface
{
 public:
    XScreenSaverInterface(Display* display)
	: m_xdisplay(display)
    {
	findScreenSaverWindow();
    }

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
    : m_impl(new InhibitScreenSaverImpl())
{
}

extern Display * get_x_display(Gtk::Widget & widget);

void InhibitScreenSaver::on_realize(Gtk::Widget* widget)
{
    m_XScreenSaverInterface = boost::make_shared<XScreenSaverInterface>(get_x_display(*widget));
}

void InhibitScreenSaver::simulateUserActivity()
{
    m_impl->simulateUserActivity();
    if (m_XScreenSaverInterface)
	m_XScreenSaverInterface->simulateUserActivity();
}

// -------------------------------------------------------------------

InhibitScreenSaverImpl::InhibitScreenSaverImpl()
    : m_index(0),
      m_connected(false)
{
    DBus::init();    

    m_dispatcher = DBus::Glib::Dispatcher::create();

    m_connection = m_dispatcher->create_connection( DBus::BUS_SESSION );

    const char* busName = "org.freedesktop.DBus";
    const char* interface = busName;
    const char* path = "/";

    m_object_DBus = m_connection->create_object_proxy(busName, path);
    m_method_DBus_GetNameOwner = DBus::MethodProxyBase::create("GetNameOwner");
    m_object_DBus->add_method(interface, m_method_DBus_GetNameOwner);

    sendGetOwnerNameRequest();
}

void InhibitScreenSaverImpl::sendGetOwnerNameRequest()
{
    DEBUG();
    DBus::CallMessage::pointer msg = m_method_DBus_GetNameOwner->create_call_message();
    msg << screenSaver[m_index].busName;
    m_pending = m_method_DBus_GetNameOwner->call_async(msg);
    m_pending->signal_notify().connect( sigc::mem_fun(this, &InhibitScreenSaverImpl::recvGetNameOwnerResponse) );
    m_connection->flush();
}

void InhibitScreenSaverImpl::recvGetNameOwnerResponse()
{
    DEBUG();
    DBus::Message::pointer retmsg = m_pending->steal_reply();
    if ( retmsg->type() == DBus::ERROR_MESSAGE )
    {
	DBus::ErrorMessage::pointer errormsg = DBus::ErrorMessage::create(retmsg);
	DEBUG(<< "DBus::ErrorMessage: " << errormsg->name());
	if (m_index+1 < numScreenSaver)
	{
	    m_index++;
	    sendGetOwnerNameRequest();
	}
	return;
    }
    std::string name;
    retmsg >> name;
    DEBUG(<< name);

    connect();
}

void InhibitScreenSaverImpl::connect()
{
    const char* busName = screenSaver[m_index].busName;
    const char* path = screenSaver[m_index].path;
    const char* interface = busName;
	
    DEBUG(<< busName << path);

    // std::string introspection = m_connection->introspect( busName, path );
    // std::cout << introspection << std::endl;

    m_object_ScreenSaver = m_connection->create_object_proxy(busName, path);

    m_method_ScreenSaver_SimulateUserActivity = DBus::MethodProxyBase::create("SimulateUserActivity");
    m_object_ScreenSaver->add_method(interface, m_method_ScreenSaver_SimulateUserActivity );

    m_connected = true;
}

void InhibitScreenSaverImpl::simulateUserActivity()
{
    if (m_connected)
	sendSimulateUserActivityRequest();
}

void InhibitScreenSaverImpl::sendSimulateUserActivityRequest()
{
    DEBUG();
    DBus::CallMessage::pointer msg = m_method_ScreenSaver_SimulateUserActivity->create_call_message();
    m_pending = m_method_ScreenSaver_SimulateUserActivity->call_async(msg);
    m_pending->signal_notify().connect( sigc::mem_fun(this, &InhibitScreenSaverImpl::recvSimulateUserActivityResponse) );
    m_connection->flush();
}

void InhibitScreenSaverImpl::recvSimulateUserActivityResponse()
{
    DEBUG();
    DBus::Message::pointer retmsg = m_pending->steal_reply();
    if ( retmsg->type() == DBus::ERROR_MESSAGE )
    {
	DBus::ErrorMessage::pointer errormsg = DBus::ErrorMessage::create(retmsg);
	ERROR(<< "DBus::ErrorMessage: " << errormsg->name());
    }
}

// -------------------------------------------------------------------

int XScreenSaverInterface::errorHandler(Display *dpy, XErrorEvent *error)
{
    if (error->error_code == BadWindow)
    {
	gotBadWindow = true;
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

	for (int i = 0; i < nchildren; i++)
	{
	    XSync (m_xdisplay, False);

	    gotBadWindow = False;
	    oldErrorHandler = XSetErrorHandler (errorHandler);

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

	    XSetErrorHandler (oldErrorHandler);
	    oldErrorHandler = 0;

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
	ERROR(<< "XSendEvent failed");
    }

    XFlush(m_xdisplay);
}

// -------------------------------------------------------------------
