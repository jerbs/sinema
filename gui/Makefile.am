## gui/Makefile.am
bin_PROGRAMS = sinema
sinema_SOURCES = About.cpp About.hpp \
		 ChannelConfigWidget.cpp ChannelConfigWidget.hpp \
		 ComboBoxDialog.hpp \
		 ConfigWindow.cpp ConfigWindow.hpp \
		 ControlWindow.cpp ControlWindow.hpp \
		 General.hpp \
		 GtkmmMediaCommon.cpp GtkmmMediaCommon.hpp \
		 GtkmmDaemonProxy.cpp GtkmmDaemonProxy.hpp \
		 GtkmmMediaPlayer.cpp GtkmmMediaPlayer.hpp \
		 GtkmmMediaRecorder.cpp GtkmmMediaRecorder.hpp \
		 GtkmmPlayList.cpp GtkmmPlayList.hpp \
		 InhibitScreenSaver.cpp InhibitScreenSaver.hpp \
		 main.cpp \
		 MainWindow.cpp MainWindow.hpp \
		 PlayerConfigWidget.cpp PlayerConfigWidget.hpp \
		 PlayListTreeModel.cpp PlayListTreeModel.hpp \
		 PlayListWindow.cpp PlayListWindow.hpp \
		 SignalDispatcher.cpp SignalDispatcher.hpp
sinema_CXXFLAGS= $(GUI_CFLAGS) -std=c++0x
sinema_LDADD   = ../receiver/libreceiver.la \
		 ../player/libplayer.la \
		 ../deinterlacer/libdeinterlacer.la \
		 ../recorder/librecorder.la \
		 ../dproxy/libdproxy.la \
		 ../common/libcommon.la \
		 ../platform/libplatform.la
sinema_LDFLAGS = $(PLAYER_LIBS) \
		 -lz -lm \
		 $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB) \
		 -lboost_serialization \
		 -L/usr/X11R6/lib -lX11 -lXext -lXv \
		 -lasound \
		 $(GUI_LIBS)

About.cpp: COPYING.inc README.inc version.inc

COPYING.inc: ../COPYING
	../tools/wrap $< $@

README.inc: ../README
	../tools/wrap $< $@

version.inc: ../version
	../tools/wrap $< $@
