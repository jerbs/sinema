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
sinema_LDFLAGS = -rdynamic $(FIRST_LDFLAGS) -L/usr/X11R6/lib $(BOOST_LDFLAGS) \
                 -Wl,-rpath,$(prefix)/lib \
                 -Wl,--enable-new-dtags

# The linker option -rpath sets DT_RPATH and --enable-new-dtags additionlly sets
# DT_RUNPATH. In this case the dynamic linker will ignore DT_RPATH. DT_RUNPATH
# can be overwritten by the user with LD_LIBRARY_PATH.
#
# See 'How To Write Shared Libraries', Ulrich Drepper,
#     section 3.9 'Inter-Object File Relations'
#
# Correct settings can be verified with:
# $ readelf -d ../root/bin/sinema | grep PATH
# 0x000000000000000f (RPATH)              Bibliothek rpath: [/home/joachim/Projekte/VideoPlayer/root/lib]
# 0x000000000000001d (RUNPATH)            Bibliothek runpath: [/home/joachim/Projekte/VideoPlayer/root/lib]
#
# Note: Using the libtool option -rpath only sets LD_RPATH.

sinema_LDADD   = ../receiver/libreceiver.la \
		 ../player/libplayer.la \
		 ../deinterlacer/libdeinterlacer.la \
		 ../recorder/librecorder.la \
		 ../dproxy/libdproxy.la \
		 ../common/libcommon.la \
		 ../platform/libplatform.la \
		 $(GUI_LIBS) $(FFMPEG_LIBS) \
		 $(BOOST_REGEX_LIB) $(BOOST_SYSTEM_LIB) \
		 $(BOOST_SYSTEM_LIB) $(BOOST_SERIALIZATION_LIB) \
		 $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB) \
		 -lX11 -lXext -lXv -lXi \
		 -lz -lm -ljpeg \
		 -lasound

About.cpp: COPYING.inc README.inc version.inc

COPYING.inc: ../COPYING
	../tools/wrap $< $@

README.inc: ../README
	../tools/wrap $< $@

version.inc: ../version
	../tools/wrap $< $@
