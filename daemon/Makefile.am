## daemon/Makefile.am
bin_PROGRAMS = sinemad
sinemad_SOURCES = Daemon.cpp Daemon.hpp \
	          main.cpp \
		  Server.cpp Server.hpp \
		  SinemadInterface.hpp
sinemad_CXXFLAGS= $(GUI_CFLAGS) -std=c++0x
sinemad_LDFLAGS = $(BOOST_LDFLAGS)
sinemad_LDADD   = ../receiver/libreceiver.la \
		  ../recorder/librecorder.la \
		  ../common/libcommon.la \
		  ../platform/libplatform.la \
		  $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB) \
		  -lboost_serialization -lboost_system

About.cpp: COPYING.inc README.inc version.inc

COPYING.inc: ../COPYING
	../tools/wrap $< $@

version.inc: ../version
	../tools/wrap $< $@
