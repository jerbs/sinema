## platform/test/Makefile.am


## Event Test
noinst_PROGRAMS = eventTest
eventTest_SOURCES = eventTest.cpp
eventTest_CPPFLAGS = $(AM_CFLAGS) \
		     $(BOOST_CPPFLAGS)
eventTest_CXXFLAGS = -std=c++0x
eventTest_LDADD = ../libplatform.la
eventTest_LDFLAGS = -lz -lm \
		    $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB)
