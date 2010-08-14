## player/Makefile.am

## Convinience library:
noinst_LTLIBRARIES = libplayer.la
libplayer_la_SOURCES = AlsaFacade.cpp AlsaFacade.hpp \
		       AlsaMixer.cpp AlsaMixer.hpp \
		       AudioDecoder.cpp AudioDecoder.hpp \
		       AudioOutput.cpp AudioOutput.hpp \
		       AudioFrame.hpp \
		       Deinterlacer.cpp Deinterlacer.hpp \
		       Demuxer.cpp Demuxer.hpp \
		       GeneralEvents.hpp \
		       MediaPlayer.cpp MediaPlayer.hpp \
		       PlayList.cpp PlayList.hpp \
		       VideoDecoder.cpp VideoDecoder.hpp \
		       VideoOutput.cpp VideoOutput.hpp \
		       XlibFacade.cpp XlibFacade.hpp \
		       XlibHelpers.cpp XlibHelpers.hpp
libplayer_la_CXXFLAGS = -std=c++0x

## Audio/Video Sync Test
noinst_PROGRAMS = synctest
synctest_SOURCES = AlsaFacade.cpp AlsaFacade.hpp \
		   AlsaMixer.cpp AlsaMixer.hpp \
		   AudioOutput.cpp AudioOutput.hpp \
		   GeneralEvents.hpp \
		   SyncTest.cpp SyncTest.hpp \
		   VideoOutput.cpp VideoOutput.hpp \
		   XlibFacade.cpp XlibFacade.hpp \
		   XlibHelpers.cpp XlibHelpers.hpp
synctest_CPPFLAGS = -DSYNCTEST $(AM_CFLAGS) $(BOOST_CPPFLAGS)
synctest_CXXFLAGS = -std=c++0x
synctest_LDADD = ../platform/libplatform.la
synctest_LDFLAGS = $(PLAYER_LIBS) \
		   -lz -lm \
		   $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) \
		   -L/usr/X11R6/lib -lX11 -lXext -lXv \
		   -lasound

## http://www.gnu.org/software/hello/manual/automake/Objects-created-both-with-libtool-and-without.html
## http://www.gnu.org/software/hello/manual/automake/Renamed-Objects.html
