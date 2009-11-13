## gui/Makefile.am

bin_PROGRAMS   = player
player_SOURCES = main.cpp ControlWindow.cpp GtkmmMediaPlayer.cpp MainWindow.cpp SignalDispatcher.cpp 
player_CXXFLAGS= $(GUI_CFLAGS)
player_LDADD   = ../player/libplayer.la ../platform/libplatform.la
player_LDFLAGS = $(PLAYER_LIBS) -lz -lm -lboost_thread-mt -L/usr/X11R6/lib -lX11 -lXext -lXv -lasound $(GUI_LIBS)
