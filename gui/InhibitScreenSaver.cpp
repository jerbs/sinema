//
// Inhibit Screen Saver
//
// Copyright (C) Joachim Erbs
//

#include "InhibitScreenSaver.hpp"
#include "platform/Logging.hpp"

#include <dbus-cxx.h>
#include <dbus-cxx-glibmm/dispatcher.h>

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
};

const InhibitScreenSaverImpl::DBusObject InhibitScreenSaverImpl::screenSaver[] = {
    {"org.freedesktop.ScreenSaver", "/ScreenSaver"},
    {"org.gnome.ScreenSaver", "/"}
};

const int InhibitScreenSaverImpl::numScreenSaver =
    sizeof(InhibitScreenSaverImpl::screenSaver)/sizeof(InhibitScreenSaverImpl::DBusObject);

// -------------------------------------------------------------------

InhibitScreenSaver::InhibitScreenSaver()
    : m_impl(new InhibitScreenSaverImpl())
{
}

void InhibitScreenSaver::simulateUserActivity()
{
    m_impl->simulateUserActivity();
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
