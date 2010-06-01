## gui/Makefile.am

## libdbus-cxx code has to be compiled with -std=c++0x
noinst_LTLIBRARIES  = libmodern.la
libmodern_la_SOURCES = InhibitScreenSaver.cpp
libmodern_la_CXXFLAGS = $(GUI_CFLAGS) -std=c++0x

bin_PROGRAMS   = player
player_SOURCES = main.cpp ChannelConfigWindow.cpp ControlWindow.cpp GtkmmMediaPlayer.cpp GtkmmMediaReceiver.cpp GtkmmMediaRecorder.cpp GtkmmMediaCommon.cpp GtkmmPlayList.cpp MainWindow.cpp PlayListTreeModel.cpp PlayListWindow.cpp SignalDispatcher.cpp 
player_CXXFLAGS= $(GUI_CFLAGS)
player_LDADD   = ../receiver/libreceiver.la ../player/libplayer.la ../recorder/librecorder.la ../common/libcommon.la ../platform/libplatform.la libmodern.la
player_LDFLAGS = $(PLAYER_LIBS) -lz -lm $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB) -L/usr/X11R6/lib -lX11 -lXext -lXv -lasound $(GUI_LIBS)
