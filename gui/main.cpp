//
// Media Player
//
// Copyright (C) Joachim Erbs, 2009
//

#include <gtkmm/main.h>
#include "gui/ControlWindow.hpp"

#ifndef SYNCTEST
#include "player/MediaPlayer.hpp"
#else
#include "player/SyncTest.hpp"
#endif

#include <iostream>

std::string file;

int main(int argc, char *argv[])
{
    Gtk::Main kit(argc, argv);

#ifndef SYNCTEST

    if (argc != 2)
    {
	std::cout << "Usage: " << argv[0] << " <file>" << std::endl;
	exit(-1);
    }

    MediaPlayer mediaPlayer;
    mediaPlayer.init();

    file = std::string(argv[1]);
    mediaPlayer.open(file);

    ControlWindow controlWindow(mediaPlayer);
    Gtk::Main::run(controlWindow);

#else

    SyncTestApp syncTestApp;
    syncTestApp();

#endif

    return 0;
}
