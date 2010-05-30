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

#undef DEBUG 
#define DEBUG(text) std::cout << __PRETTY_FUNCTION__ text << std::endl;

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

    Display* m_xdisplay;
};

const InhibitScreenSaverImpl::DBusObject InhibitScreenSaverImpl::screenSaver[] = {
    {"org.freedesktop.ScreenSaver", "/ScreenSaver"},
    {"org.gnome.ScreenSaver", "/"}
};

const int InhibitScreenSaverImpl::numScreenSaver =
    sizeof(InhibitScreenSaverImpl::screenSaver)/sizeof(InhibitScreenSaverImpl::DBusObject);

// -------------------------------------------------------------------

const char *progname = "VideoPlayer";
Atom XA_SCREENSAVER, XA_SCREENSAVER_VERSION, XA_SCREENSAVER_RESPONSE;
Atom XA_SCREENSAVER_ID, XA_SCREENSAVER_STATUS, XA_EXIT;
Atom XA_VROOT, XA_SELECT, XA_DEMO, XA_BLANK, XA_LOCK;



#include "remote.c"

InhibitScreenSaver::InhibitScreenSaver()
    : m_impl(new InhibitScreenSaverImpl())
{
}

extern Display * get_x_display(Gtk::Widget & widget);

void InhibitScreenSaver::on_realize(Gtk::Widget* widget)
{
    m_impl->m_xdisplay = get_x_display(*widget);
    Display* dpy = m_impl->m_xdisplay;

    XA_VROOT = XInternAtom (dpy, "__SWM_VROOT", False);
    XA_SCREENSAVER = XInternAtom (dpy, "SCREENSAVER", False);
    XA_SCREENSAVER_ID = XInternAtom (dpy, "_SCREENSAVER_ID", False);
    XA_SCREENSAVER_VERSION = XInternAtom (dpy, "_SCREENSAVER_VERSION",False);
    XA_SCREENSAVER_STATUS = XInternAtom (dpy, "_SCREENSAVER_STATUS", False);
    XA_SCREENSAVER_RESPONSE = XInternAtom (dpy, "_SCREENSAVER_RESPONSE", False);
    //XA_ACTIVATE = XInternAtom (dpy, "ACTIVATE", False);
    //XA_DEACTIVATE = XInternAtom (dpy, "DEACTIVATE", False);
    //XA_RESTART = XInternAtom (dpy, "RESTART", False);
    //XA_CYCLE = XInternAtom (dpy, "CYCLE", False);
    //XA_NEXT = XInternAtom (dpy, "NEXT", False);
    //XA_PREV = XInternAtom (dpy, "PREV", False);
    XA_SELECT = XInternAtom (dpy, "SELECT", False);
    XA_EXIT = XInternAtom (dpy, "EXIT", False);
    XA_DEMO = XInternAtom (dpy, "DEMO", False);
    //XA_PREFS = XInternAtom (dpy, "PREFS", False);
    XA_LOCK = XInternAtom (dpy, "LOCK", False);
    XA_BLANK = XInternAtom (dpy, "BLANK", False);
    //XA_THROTTLE = XInternAtom (dpy, "THROTTLE", False);
    //XA_UNTHROTTLE = XInternAtom (dpy, "UNTHROTTLE", False);

}

void InhibitScreenSaver::simulateUserActivity()
{
    if (m_impl->m_xdisplay)
    {
	char* error_msg;
	Atom XA_DEACTIVATE = XInternAtom(m_impl->m_xdisplay, "DEACTIVATE", False);
	xscreensaver_command(m_impl->m_xdisplay, XA_DEACTIVATE, 0, true, 0);
    }

    m_impl->simulateUserActivity();
}

// -------------------------------------------------------------------

InhibitScreenSaverImpl::InhibitScreenSaverImpl()
    : m_index(0),
      m_connected(false),
      m_xdisplay(0)
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
