## gui/Makefile.am

bin_PROGRAMS   = player
player_SOURCES = main.cpp
player_LDADD   = ../player/libplayer.la ../platform/libplatform.la
player_LDFLAGS = -lavutil -lavformat -lavcodec -lswscale -lz -lm -lboost_thread-mt -L/usr/X11R6/lib -lX11 -lXext -lXv -lasound