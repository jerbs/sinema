## gui/Makefile.am

bin_PROGRAMS   = player
player_SOURCES = main.cpp ChannelConfigWindow.cpp ControlWindow.cpp GtkmmMediaPlayer.cpp GtkmmMediaReceiver.cpp MainWindow.cpp SignalDispatcher.cpp 
player_CXXFLAGS= $(GUI_CFLAGS)
player_LDADD   = ../receiver/libreceiver.la ../player/libplayer.la ../platform/libplatform.la
player_LDFLAGS = $(PLAYER_LIBS) -lz -lm -lboost_thread-mt -L/usr/X11R6/lib -lX11 -lXext -lXv -lasound $(GUI_LIBS)
