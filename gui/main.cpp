//
// Media Player
//
// Copyright (C) Joachim Erbs, 2009
//

#include <gtkmm/main.h>
#include <gtkmm/window.h>

#ifndef SYNCTEST
#include "player/MediaPlayer.hpp"
#else
#include "player/SyncTest.hpp"
#endif

#include <iostream>

int main(int argc, char *argv[])
{
    Gtk::Main kit(argc, argv);
    Gtk::Window window;

#ifndef SYNCTEST

    if (argc != 2)
    {
	std::cout << "Usage: " << argv[0] << " <file>" << std::endl;
	exit(-1);
    }

    MediaPlayer mediaPlayer;
    mediaPlayer(argv[1]);

#else

    SyncTestApp syncTestApp;
    syncTestApp();
    
#endif

    Gtk::Main::run(window);
    
    return 0;
}
