## player/Makefile.am

## Convinience library:
noinst_LTLIBRARIES    = libplayer.la
libplayer_la_SOURCES = AlsaFacade.cpp AlsaMixer.cpp AudioDecoder.cpp AudioOutput.cpp Demuxer.cpp MediaPlayer.cpp PlayList.cpp VideoDecoder.cpp VideoOutput.cpp XlibFacade.cpp XlibHelpers.cpp

## Audio/Video Sync Test
bin_PROGRAMS = synctest
synctest_SOURCES = AlsaFacade.cpp AlsaMixer.cpp AudioOutput.cpp SyncTest.cpp VideoOutput.cpp XlibFacade.cpp XlibHelpers.cpp
synctest_CPPFLAGS = -DSYNCTEST $(AM_CFLAGS) $(BOOST_CPPFLAGS)
synctest_LDADD = ../platform/libplatform.la
synctest_LDFLAGS = $(PLAYER_LIBS) -lz -lm $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) -L/usr/X11R6/lib -lX11 -lXext -lXv -lasound

## http://www.gnu.org/software/hello/manual/automake/Objects-created-both-with-libtool-and-without.html
## http://www.gnu.org/software/hello/manual/automake/Renamed-Objects.html
