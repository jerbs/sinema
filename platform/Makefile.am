## platform/Makefile.am

SUBDIRS = . test

## Convinience library:
noinst_LTLIBRARIES = libplatform.la
libplatform_la_SOURCES = concurrent_queue.hpp \
			 event_processor.hpp \
			 event_receiver.hpp \
			 Logging.cpp Logging.hpp \
			 temp_value.hpp \
			 timer.hpp
libplatform_la_CXXFLAGS = -std=c++0x
