//
// Media Player
//
// Copyright (C) Joachim Erbs, 2009
//

#include <iostream>
#include <boost/make_shared.hpp>
#include <gtkmm/main.h>

#include "gui/ControlWindow.hpp"
#include "gui/GtkmmMediaPlayer.hpp"
#include "gui/MainWindow.hpp"
#include "gui/SignalDispatcher.hpp"

#ifdef SYNCTEST
#include "player/SyncTest.hpp"
#endif

 
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

    GtkmmMediaPlayer mediaPlayer(playList);
    mediaPlayer.init();
    mediaPlayer.open();

    SignalDispatcher signalDispatcher(mediaPlayer);

    ControlWindow controlWindow(mediaPlayer, signalDispatcher);
    controlWindow.show();

    MainWindow mainWindow(signalDispatcher);
    Gtk::Main::run(mainWindow);

#else

    SyncTestApp syncTestApp;
    syncTestApp();

#endif

    return 0;
}
