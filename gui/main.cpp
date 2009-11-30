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

    if (argc < 1)
    {
	std::cout << "Usage: " << argv[0] << " [<file1> <file2> ... <filen>]" << std::endl;
	exit(-1);
    }

    boost::shared_ptr<PlayList> playList(new PlayList());
    for (int i=1; i<argc; i++)
	playList->append(std::string(argv[i]));

    GtkmmMediaPlayer mediaPlayer(playList);
    mediaPlayer.init();
    SignalDispatcher signalDispatcher(mediaPlayer);
    ControlWindow controlWindow(mediaPlayer, signalDispatcher);
    MainWindow mainWindow(mediaPlayer, signalDispatcher);

    controlWindow.set_transient_for(mainWindow);
    controlWindow.signal_window_state_event().connect(sigc::mem_fun(signalDispatcher,
				    &SignalDispatcher::on_control_window_state_event));

    mainWindow.signal_button_press_event().connect(sigc::mem_fun(signalDispatcher,
				    &SignalDispatcher::on_button_press_event));
    mainWindow.signal_window_state_event().connect(sigc::mem_fun(signalDispatcher,
				    &SignalDispatcher::on_main_window_state_event));

    signalDispatcher.zoomMainWindow.connect(sigc::mem_fun(mainWindow,
				    &MainWindow::zoom));
    signalDispatcher.setMainWindow(&mainWindow);

    mediaPlayer.notificationVideoSize.connect( sigc::mem_fun(&mainWindow, &MainWindow::on_notification_video_size) );


    Gtk::Main::run(mainWindow);

#else

    SyncTestApp syncTestApp;
    syncTestApp();

#endif

    return 0;
}
