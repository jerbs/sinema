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
		       JpegWriter.cpp JpegWriter.hpp \
		       MediaPlayer.cpp MediaPlayer.hpp \
		       PlayList.cpp PlayList.hpp \
		       VideoDecoder.cpp VideoDecoder.hpp \
		       VideoOutput.cpp VideoOutput.hpp \
		       XlibFacade.cpp XlibFacade.hpp \
		       XlibHelpers.cpp XlibHelpers.hpp
libplayer_la_CPPFLAGS = $(AM_CFLAGS) $(BOOST_CPPFLAGS) $(FFMPEG_CFLAGS)
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
synctest_CPPFLAGS = -DSYNCTEST $(AM_CFLAGS) $(BOOST_CPPFLAGS) $(FFMPEG_CFLAGS)
synctest_CXXFLAGS = -std=c++0x
synctest_LDFLAGS = $(BOOST_LDFLAGS) -L/usr/X11R6/lib 
synctest_LDADD = ../platform/libplatform.la \
		 $(FFMPEG_LIBS) \
		 $(BOOST_THREAD_LIB) \
		 -lX11 -lXext -lXv \
		 -lz -lm \
		 -lasound

## http://www.gnu.org/software/hello/manual/automake/Objects-created-both-with-libtool-and-without.html
## http://www.gnu.org/software/hello/manual/automake/Renamed-Objects.html
