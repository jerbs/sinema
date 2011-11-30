## platform/test/Makefile.am

noinst_PROGRAMS = eventTest client server

## Event Test
eventTest_SOURCES = eventTest.cpp
eventTest_CPPFLAGS = $(AM_CFLAGS) \
		     $(BOOST_CPPFLAGS)
eventTest_CXXFLAGS = -std=c++0x
eventTest_LDFLAGS = $(BOOST_LDFLAGS)
eventTest_LDADD = ../libplatform.la \
		  $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB) \
		  -lz -lm

## client
client_SOURCES = client.cpp \
		 ClientServerEvents.hpp \
		 ClientServerInterface.hpp
client_CPPFLAGS = $(AM_CFLAGS) \
		  $(BOOST_CPPFLAGS)
client_CXXFLAGS = -std=c++0x
client_LDFLAGS = $(BOOST_LDFLAGS)
client_LDADD = ../libplatform.la \
               $(BOOST_SYSTEM_LIB) $(BOOST_SERIALIZATION_LIB) \
               $(BOOST_THREAD_LIB)

## server
server_SOURCES = server.cpp \
		 ClientServerEvents.hpp \
		 ClientServerInterface.hpp
server_CPPFLAGS = $(AM_CFLAGS) \
		  $(BOOST_CPPFLAGS)
server_CXXFLAGS = -std=c++0x
server_LDFLAGS = $(BOOST_LDFLAGS)
server_LDADD = ../libplatform.la \
               $(BOOST_SYSTEM_LIB) $(BOOST_SERIALIZATION_LIB) \
               $(BOOST_THREAD_LIB)
