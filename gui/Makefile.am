## gui/Makefile.am

bin_PROGRAMS   = player
player_SOURCES = main.cpp ConfigWindow.cpp ChannelConfigWidget.cpp ControlWindow.cpp GtkmmMediaPlayer.cpp GtkmmMediaReceiver.cpp GtkmmMediaRecorder.cpp GtkmmMediaCommon.cpp GtkmmPlayList.cpp InhibitScreenSaver.cpp MainWindow.cpp PlayerConfigWidget.cpp PlayListTreeModel.cpp PlayListWindow.cpp SignalDispatcher.cpp 
player_CXXFLAGS= $(GUI_CFLAGS) -std=c++0x
player_LDADD   = ../receiver/libreceiver.la ../player/libplayer.la ../deinterlacer/libdeinterlacer.la ../recorder/librecorder.la ../common/libcommon.la ../platform/libplatform.la
player_LDFLAGS = $(PLAYER_LIBS) -lz -lm $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB) -L/usr/X11R6/lib -lX11 -lXext -lXv -lasound $(GUI_LIBS)
