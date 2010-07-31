## gui/Makefile.am

bin_PROGRAMS   = sinema
sinema_SOURCES = main.cpp About.cpp ConfigWindow.cpp ChannelConfigWidget.cpp ControlWindow.cpp GtkmmMediaPlayer.cpp GtkmmMediaReceiver.cpp GtkmmMediaRecorder.cpp GtkmmMediaCommon.cpp GtkmmPlayList.cpp InhibitScreenSaver.cpp MainWindow.cpp PlayerConfigWidget.cpp PlayListTreeModel.cpp PlayListWindow.cpp SignalDispatcher.cpp 
sinema_CXXFLAGS= $(GUI_CFLAGS) -std=c++0x
sinema_LDADD   = ../receiver/libreceiver.la ../player/libplayer.la ../deinterlacer/libdeinterlacer.la ../recorder/librecorder.la ../common/libcommon.la ../platform/libplatform.la
sinema_LDFLAGS = $(PLAYER_LIBS) -lz -lm $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB) -L/usr/X11R6/lib -lX11 -lXext -lXv -lasound $(GUI_LIBS)

About.cpp: COPYING.inc version.inc

COPYING.inc: ../COPYING
	../tools/wrap $< $@

version.inc: ../version
	../tools/wrap $< $@
