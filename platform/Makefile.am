## platform/Makefile.am

SUBDIRS = . test

## Convinience library:
noinst_LTLIBRARIES = libplatform.la
libplatform_la_SOURCES = concurrent_queue.hpp \
			 event_processor.hpp \
			 event_receiver.hpp \
			 interface.hpp \
			 Logging.cpp Logging.hpp \
			 process_starter.hpp \
			 tcp_acceptor.hpp \
			 tcp_client.hpp \
			 tcp_connection.hpp \
			 tcp_connector.hpp \
			 tcp_server.hpp \
			 temp_value.hpp \
			 timer.hpp
libplatform_la_CXXFLAGS = -std=c++0x
libplatform_la_LDFLAGS = -lrt
