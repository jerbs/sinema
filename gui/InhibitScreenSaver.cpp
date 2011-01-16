//
// Inhibit Screen Saver
//
// Copyright (C) Joachim Erbs, 2010-2011
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

#include "InhibitScreenSaver.hpp"
#include "platform/Logging.hpp"

#include <gio/gio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

// #undef TRACE_DEBUG 
// #define TRACE_DEBUG(text) std::cout << __PRETTY_FUNCTION__ text << std::endl;

// -------------------------------------------------------------------

class DBusScreenSaverInterface
{
public:
    DBusScreenSaverInterface();
    ~DBusScreenSaverInterface();

    void simulateUserActivity();

private:
    template<typename C, void(C::*M)(GObject*, GAsyncResult*)>
    static void wrapper(GObject *source_object,
			GAsyncResult *res,
			gpointer user_data)
    {
	C* obj = (C*)user_data;
	(obj->*M)(source_object, res);
    }

    struct DBusMethod
    {
	const gchar* bus_name;
	const gchar* interface_name;
	const gchar* object_path;
	const gchar* method_name;
    };

    GDBusProxy* proxyDBusDaemon;
    GDBusProxy* proxyScreenSaver;
    const DBusMethod* screenSaver;

    const DBusMethod dbusDaemon;
    const DBusMethod gnomeScreenSaver;
    const DBusMethod kdeScreenSaver;

    void finishProxyDBusDaemonNew(GObject*,  // source_object
				  GAsyncResult *res);
    void sendDBusDaemonGetNameOwner();
    void finishDBusDaemonGetNameOwner(GObject*,  // source_object
				      GAsyncResult *res);
    void finishProxyScreenSaverNew(GObject*,  // source_object
				   GAsyncResult *res);
};

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
    m_DBusScreenSaverInterface = boost::make_shared<DBusScreenSaverInterface>();
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

    if (m_DBusScreenSaverInterface)
	m_DBusScreenSaverInterface->simulateUserActivity();
}

// -------------------------------------------------------------------
// Determine which ScreenSaver interface has to be used (Gnome or KDE):
//   gdbus introspect --session --dest org.freedesktop.DBus --object-path /
//   gdbus call --session --dest org.freedesktop.DBus --object-path / --method org.freedesktop.DBus.GetNameOwner org.gnome.ScreenSaver
//   gdbus call --session --dest org.freedesktop.DBus --object-path / --method org.freedesktop.DBus.GetNameOwner org.freedesktop.ScreenSaver
//
// Accessing the ScreenSaver:
//   gdbus introspect --session --dest org.gnome.ScreenSaver --object-path /
//   gdbus introspect --session --dest org.freedesktop.ScreenSaver --object-path /ScreenSaver
//   gdbus call --session --dest org.gnome.ScreenSaver --object-path / --method org.gnome.ScreenSaver.SimulateUserActivity
//   gdbus call --session --dest org.freedesktop.ScreenSaver  --object-path /ScreenSaver org.freedesktop.ScreenSaver.SimulateUserActivity
//   At least the Gnome screen saver does not send a response for SimulateUserActivity.
//
// Monitoring DBus:
//   gdbus monitor --session --dest org.gnome.ScreenSaver --object-path /
//
// Documentation:
//   gdbus documentation: http://library.gnome.org/devel/gio/2.26/gdbus.html

DBusScreenSaverInterface::DBusScreenSaverInterface()
    : proxyDBusDaemon(0),
      proxyScreenSaver(0),
      screenSaver(0),
      dbusDaemon
	 {"org.freedesktop.DBus",
	  "org.freedesktop.DBus",
	  "/",
	  "org.freedesktop.DBus.GetNameOwner"},
      gnomeScreenSaver
	 {"org.gnome.ScreenSaver",
	  "org.gnome.ScreenSaver",
	  "/",
	  "org.gnome.ScreenSaver.SimulateUserActivity"},
      kdeScreenSaver
	 {"org.freedesktop.ScreenSaver",
	  "org.freedesktop.ScreenSaver",
	  "/ScreenSaver",
	  "org.freedesktop.ScreenSaver.SimulateUserActivity"}
{
    g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION,
			     G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
			     NULL,  // GDBusInterfaceInfo*
			     dbusDaemon.bus_name,
			     dbusDaemon.object_path,
			     dbusDaemon.interface_name,
			     NULL, // GCancellable*
			     &wrapper<DBusScreenSaverInterface,
			              &DBusScreenSaverInterface::finishProxyDBusDaemonNew>,
			     this);
}

DBusScreenSaverInterface::~DBusScreenSaverInterface()
{
    // Fixme: Cancel open asynchronous operations before deleting the object.

    if (proxyDBusDaemon)
	g_object_unref(proxyDBusDaemon);

    if (proxyScreenSaver)
	g_object_unref(proxyScreenSaver);
}

void DBusScreenSaverInterface::finishProxyDBusDaemonNew(GObject*,  // source_object
							GAsyncResult *res)
{
    GError* error = NULL;
    proxyDBusDaemon = g_dbus_proxy_new_for_bus_finish(res, &error);
    if (proxyDBusDaemon)
    {
	TRACE_DEBUG(<< "ok");
	screenSaver = &kdeScreenSaver;
	sendDBusDaemonGetNameOwner();
    }
    else
    {
	TRACE_ERROR(<< "failed");
    }
}

void DBusScreenSaverInterface::sendDBusDaemonGetNameOwner()
{
    TRACE_DEBUG(<< screenSaver->bus_name);
    g_dbus_proxy_call(proxyDBusDaemon,
		      dbusDaemon.method_name,
		      g_variant_new ("(s)", screenSaver->bus_name),
		      G_DBUS_CALL_FLAGS_NO_AUTO_START,
		      500,   // timeout in milliseconds, -1 use the proxy default timeout
		      NULL,  //  GCancellable*
		      &wrapper<DBusScreenSaverInterface,
		               &DBusScreenSaverInterface::finishDBusDaemonGetNameOwner>,
		      this);
}

void DBusScreenSaverInterface::finishDBusDaemonGetNameOwner(GObject*,  // source_object
							    GAsyncResult *res)
{
    // GDBusProxy* proxy = (GDBusProxy*)source_object;
    GError* error = NULL;
    GVariant* gvariant = g_dbus_proxy_call_finish(proxyDBusDaemon, res, &error);

    if (error != NULL)
    {
	// Don't print this as an error message:
	TRACE_DEBUG(<< "Error: domain: " << error->domain
		    << ", code: " << error->code
		    << ", message: " << error->message);
	g_error_free(error);

	if (screenSaver == &kdeScreenSaver)
	{
	    screenSaver = &gnomeScreenSaver;
	    sendDBusDaemonGetNameOwner();
	}
	else
	{
	    // No DBus screen saver found.
	}
    }
    else
    {
	TRACE_DEBUG(<< "found " << screenSaver->bus_name);
	if (gvariant)
	{
	    g_variant_unref(gvariant);
	}

	// Create DBus proxy for screen saver.
	g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION,
				 G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
				 NULL,  // GDBusInterfaceInfo*
				 screenSaver->bus_name,
				 screenSaver->object_path,
				 screenSaver->interface_name,
				 NULL, // GCancellable*
				 &wrapper<DBusScreenSaverInterface,
				          &DBusScreenSaverInterface::finishProxyScreenSaverNew>,
				 this);
    }
}

void DBusScreenSaverInterface::finishProxyScreenSaverNew(GObject*,  // source_object
							 GAsyncResult *res)
{
    TRACE_DEBUG();

    GError* error = NULL;
    proxyScreenSaver = g_dbus_proxy_new_for_bus_finish(res, &error);
    if (error != NULL)
    {
	TRACE_ERROR(<< "domain: " << error->domain
		    << ", code: " << error->code
		    << ", message: " << error->message);
	g_error_free(error);
    }
    else if (proxyScreenSaver)
    {
	TRACE_DEBUG(<< "ok");
    }
    else
    {
	TRACE_ERROR(<< "failed");
    }
}

void DBusScreenSaverInterface::simulateUserActivity()
{
    if (proxyScreenSaver)
    {
	TRACE_DEBUG();
	// SimulateUserActivity has no output parameters. The screen saver
	// will not send a response.
	g_dbus_proxy_call(proxyScreenSaver,
			  screenSaver->method_name,
			  NULL,  // No parameters
			  G_DBUS_CALL_FLAGS_NO_AUTO_START,
			  500,   // timeout in milliseconds, -1 use the proxy default timeout
			  NULL,  // GCancellable*
			  NULL,  // No response
			  this);
    }
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
