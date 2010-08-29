## platform/test/Makefile.am

noinst_PROGRAMS = eventTest client server

## Event Test
eventTest_SOURCES = eventTest.cpp
eventTest_CPPFLAGS = $(AM_CFLAGS) \
		     $(BOOST_CPPFLAGS)
eventTest_CXXFLAGS = -std=c++0x
eventTest_LDADD = ../libplatform.la
eventTest_LDFLAGS = -lz -lm \
		    $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB)

## client
client_SOURCES = client.cpp
client_CPPFLAGS = $(AM_CFLAGS) \
		  $(BOOST_CPPFLAGS)
client_CXXFLAGS = -std=c++0x
client_LDADD = ../libplatform.la
client_LDFLAGS = $(BOOST_LDFLAGS) -lboost_system $(BOOST_THREAD_LIB)

## server
server_SOURCES = server.cpp
server_CPPFLAGS = $(AM_CFLAGS) \
		  $(BOOST_CPPFLAGS)
server_CXXFLAGS = -std=c++0x
server_LDADD = ../libplatform.la
server_LDFLAGS = $(BOOST_LDFLAGS) -lboost_system $(BOOST_THREAD_LIB)

