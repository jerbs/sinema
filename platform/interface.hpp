//
// Inter Task Communication - interface
//
// Copyright (C) Joachim Erbs, 2010
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef PLATFORM_INTERFACE_HPP
#define PLATFORM_INTERFACE_HPP

#include <iostream>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/clear.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/remove_if.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/type_traits/is_same.hpp>

namespace itf {

// -------------------------------------------------------------------
//
// This file provides template meta code to define message interfaces for client
// server communication. A library user can implement a client server communication
// by defining a type list containing the exchanged messages. This typelist is
// used as template parameter to setup proxy objects providing a transparent
// typesafe communication. 
//
// Terminology:
//
// Event: An Event can be any user supplied type used to transfer information.
//        The event_receiver and the event_processor classes provide the
//        mechanisms needed for typesafe inter-thread communication. With the 
//        template code in this file and proxy implementations like tcp_client, 
//        tcp_server and tcp_connection a similar typesafe communication 
//        mechanism can be setup between different processes or even between
//        different hosts.
//        Event objects are allocated on the heap. boost::shared pointers or
//        std::unique pointers to these objects are used to transfer the
//        events.
//
// Procedure: A Procedure is a pair of Events. The first Event is sent from the
//            client to the server. The second event from the server to the client.
//            itf::none can be used if the procedure only has one of the two
//            events. For a symmetric procedure the same Event can be used for 
//            both directions.
//
// Interface: An Interface definition is a list of procedures. Using the same
//            Event twice for the same direction, i.e. in two different procedures,
//            is not allowed for an interface.
//
//            On an interface the template code in this file file assigns a unique
//            number to each procedure.
//
// Message: A Message is a pair of two types. An Event and an integral type wrapper
//          containing the unique number of the procedure the event belongs to.
//
// MessageList: A MessageList is a list of Message types for either the direction
//              from client to server or for the direction from server to client.
//
// -------------------------------------------------------------------

// procedure<EventSentToServer,EventSentToClient>::type
//     A metafunction getting two events and returning them as mpl::vector.
//     The library user builds an interface definition with this metafunction.

template <typename EventSentToServer,
	  typename EventSentToClient>
struct procedure
    : boost::mpl::vector<EventSentToServer,
			 EventSentToClient>
{};

// none is a special Event used by the library user to indicate that a Procedure 
// has no message for one direction.

struct none
{};

// -------------------------------------------------------------------
// message<Event,n>
//     A template class creating a Message (see above).

template <typename Event, int N>
struct message
{
    typedef boost::mpl::int_<N> msg_id;
    typedef Event type;
};

// -------------------------------------------------------------------
// Template meta-code adding a third column with a unique number to the Interface
// definition provided by the user.

template<typename SEQ, bool first> struct get_prev_index;

template<typename SEQ>
struct get_prev_index<SEQ, true>
{
    typedef boost::mpl::int_<0>::type type;
};

template<typename SEQ>
struct get_prev_index<SEQ, false>
{
    typedef typename boost::mpl::back<SEQ>::type last_elem;
    typedef typename boost::mpl::back<last_elem>::type type;
};

struct add_number_op
{
    template<typename SEQ,   // Partial sequence
	     typename ELEM>  // New element to be added to the sequence
    struct apply
    {
	static bool const first = boost::mpl::empty<SEQ>::type::value;
	typedef typename get_prev_index<SEQ, first>::type last_msg_id;
	typedef typename boost::mpl::next<last_msg_id>::type msg_id;
	typedef typename boost::mpl::push_back<ELEM, msg_id>::type elem_with_msg_id;
	typedef typename boost::mpl::push_back<SEQ, elem_with_msg_id>::type type;
    };
};

// numbered<Interface>::type
//     A metafunction assigning a unique number to each Procedure in the 
//     Interface definition.

template<typename Interface>
struct numbered
{
    typedef typename boost::mpl::fold<
	typename Interface::type,                                     // Input sequence
	typename boost::mpl::clear<typename Interface::type>::type,   // Initial output sequence
	add_number_op
    >::type type;
};

// -------------------------------------------------------------------
// Template meta-code to create a MessageList for a single direction 
// on the interface.

// get_message<Procedure, Column>::type
//     A metafunction returning a Message object for the first or 
//     the second event of the procedure.

template<typename Procedure, typename Column>
struct get_message
{
    typedef typename boost::mpl::at<
	Procedure,
	Column
    >::type dl_msg;

    typedef typename boost::mpl::back<Procedure>::type msg_id;

    typedef message<dl_msg, msg_id::value> type;
};

// is_none::apply<Message>
//     A metafunction class returning true if Message contains the 'none' Event.

struct is_none
{
    template<typename Message>
    struct apply
    {
	typedef typename boost::is_same<typename Message::type, none>::type type;
    };
};

// message_list<Interface, Column>::type
//     A metafunction returning a MessageList for the first or the second 
//     column of the Interface definition. The none Events are not part of
//     the returned list.

template<typename Interface, typename Column>
struct message_list
{
    typedef typename boost::mpl::transform<typename numbered<Interface>::type,
					   get_message<boost::mpl::placeholders::_1,
						       Column>
    >::type list;

    typedef typename boost::mpl::remove_if<
	list,
	is_none
    >::type type;
};

typedef boost::mpl::int_<0>::type firstColumn;
typedef boost::mpl::int_<1>::type secondColumn;

struct ServerSide{};
struct ClientSide{};

struct Tx{};
struct Rx{};

template<typename Side, typename TxRx> struct get_column;
template<> struct get_column<ClientSide, Tx> {typedef firstColumn  type;};
template<> struct get_column<ClientSide, Rx> {typedef secondColumn type;};
template<> struct get_column<ServerSide, Tx> {typedef secondColumn type;};
template<> struct get_column<ServerSide, Rx> {typedef firstColumn  type;};

// get_message_list<Interface, ServerSide|ClientSide, Tx|Rx>::type
//     A metafunction used by proxy implementations to get a client or server
//     side tx or rx MessageList for Interface.

template<typename Interface, typename Side, typename TxRx>
struct get_message_list
{
    typedef typename message_list<
	typename Interface::type,
	typename get_column<
	    Side,
	    TxRx
	>::type
    >::type type;
};

// -------------------------------------------------------------------

// is_same_event<Event>::apply<Message>::type
//     A metafunction class returning true when Message contains Event.

template<typename Event>
struct is_same_event
{
    template<typename Message>
    struct apply
    {
	typedef typename boost::is_same<typename Message::type, Event>::type type;
    };
};

// get_event::apply<Message>::type
//     A metafunction class returning the event contained in the Message.

struct get_event
{
    template<typename Message>
    struct apply
    {
	typedef typename Message::type type;
    };
};

// enable_if_msg_in_list<Event, MessageList, ReturnType>
//     A metafunction to enable a template member method in a proxy, when Event
//     is contained in MessageList. For all other types it is not possible to
//     instantiate the template member method.

//     Metafunction forwarding is used. Having an additional typedef in this 
//     class to access enable_if<>::type results in a compiler failure when
//     used together with function overloading. See [MPL Book, 9.9 Explicitly
//     Managing the Overload Set] SFINAE (Substitution Failure Is Not An Error).

template<typename Event,
	 typename MessageList,
	 typename ReturnType>
struct enable_if_msg_in_list
    : boost::enable_if<
          typename boost::mpl::contains<
	      typename boost::mpl::transform_view<
	          MessageList,
	          get_event
	      >::type,
	      Event
	  >,
	  ReturnType
      >
{};

// get_message_id<Event, MessageList>::type::value
//     An metafunction to determine the unique id assigned to Event. Proxy 
//     template classes are using this id to pass type information to its peer.

template<typename Event,
	 typename MessageList>
struct get_message_id
{
    typedef typename boost::mpl::deref<
	typename boost::mpl::find_if<
	    MessageList,
	    is_same_event<Event>
        >::type
    >::type::msg_id type;
	
};

// -------------------------------------------------------------------

}

#endif
