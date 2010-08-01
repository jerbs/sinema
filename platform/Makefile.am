## platform/Makefile.am

## Convinience library:
noinst_LTLIBRARIES = libplatform.la
libplatform_la_SOURCES = concurrent_queue.hpp \
			 event_processor.hpp \
			 event_receiver.hpp \
			 eventTest.cpp \
			 Logging.cpp Logging.hpp \
			 temp_value.hpp \
			 timer.hpp
libplatform_la_CXXFLAGS = -std=c++0x

## Event Test
bin_PROGRAMS = eventTest
eventTest_SOURCES = eventTest.cpp Logging.cpp
eventTest_CPPFLAGS = $(AM_CFLAGS) \
		     $(BOOST_CPPFLAGS) \
		     -std=c++0x
eventTest_LDFLAGS = -lz -lm \
		    $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB)
