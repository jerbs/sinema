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
#include <boost/make_shared.hpp>

int main(int argc, char *argv[])
{
    Gtk::Main kit(argc, argv);

#ifndef SYNCTEST

    if (argc < 2)
    {
	std::cout << "Usage: " << argv[0] << " <file1> [<file2> ... <filen>]" << std::endl;
	exit(-1);
    }

    boost::shared_ptr<PlayList> playList(new PlayList());
    for (int i=1; i<argc; i++)
	playList->append(std::string(argv[i]));

    MediaPlayer mediaPlayer(playList);
    mediaPlayer.init();
    mediaPlayer.open();

    ControlWindow controlWindow(mediaPlayer);
    Gtk::Main::run(controlWindow);

#else

    SyncTestApp syncTestApp;
    syncTestApp();

#endif

    return 0;
}
