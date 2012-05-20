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
		 KeyCodes.c KeyCodes.h \
		 main.cpp \
		 MainWindow.cpp MainWindow.hpp \
		 PlayerConfigWidget.cpp PlayerConfigWidget.hpp \
		 VideoAttributeWidget.cpp VideoAttributeWidget.hpp \
		 PlayListTreeModel.cpp PlayListTreeModel.hpp \
		 PlayListWindow.cpp PlayListWindow.hpp \
		 RemoteControl.cpp RemoteControl.hpp \
		 X11InputDevice.cpp X11InputDevice.hpp \
		 SignalDispatcher.cpp SignalDispatcher.hpp
sinema_CXXFLAGS= $(GUI_CFLAGS) -std=c++0x
sinema_LDFLAGS = -rdynamic -L/usr/X11R6/lib $(BOOST_LDFLAGS)
sinema_LDADD   = ../receiver/libreceiver.la \
		 ../player/libplayer.la \
		 ../deinterlacer/libdeinterlacer.la \
		 ../recorder/librecorder.la \
		 ../dproxy/libdproxy.la \
		 ../common/libcommon.la \
		 ../platform/libplatform.la \
		 $(GUI_LIBS) $(PLAYER_LIBS) \
		 $(BOOST_REGEX_LIB) \
		 $(BOOST_SYSTEM_LIB) $(BOOST_SERIALIZATION_LIB) \
		 $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB) \
		 -lX11 -lXext -lXv \
		 -lz -lm \
		 -lasound

About.cpp: COPYING.inc README.inc version.inc

COPYING.inc: ../COPYING
	../tools/wrap $< $@

README.inc: ../README
	../tools/wrap $< $@

version.inc: ../version
	../tools/wrap $< $@
