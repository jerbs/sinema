//
// Media Player
//
// Copyright (C) Joachim Erbs, 2009, 2010
//

#include <iostream>
#include <boost/make_shared.hpp>
#include <gtkmm/main.h>

#include "gui/ChannelConfigWindow.hpp"
#include "gui/ControlWindow.hpp"
#include "gui/GtkmmMediaPlayer.hpp"
#include "gui/GtkmmMediaReceiver.hpp"
#include "gui/GtkmmMediaCommon.hpp"
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
    GtkmmMediaReceiver mediaReceiver;
    mediaReceiver.init();
    GtkmmMediaCommon mediaCommon;
    SignalDispatcher signalDispatcher(mediaPlayer);
    ControlWindow controlWindow(mediaPlayer, signalDispatcher);
    ChannelConfigWindow channelConfigWindow;
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
    signalDispatcher.ignoreWindowResize.connect(sigc::mem_fun(mainWindow,
				    &MainWindow::ignoreWindowResize));
    signalDispatcher.setMainWindow(&mainWindow);

    mediaPlayer.notificationVideoSize.connect( sigc::mem_fun(&mainWindow,
				    &MainWindow::on_notification_video_size) );
    mediaPlayer.notificationDuration.connect( sigc::mem_fun(&mainWindow,
				    &MainWindow::set_duration) );
    mediaPlayer.notificationCurrentTime.connect( sigc::mem_fun(&mainWindow,
				    &MainWindow::set_time) );
    mediaPlayer.notificationFileName.connect( sigc::mem_fun(&mainWindow,
				    &MainWindow::set_title) );

    mediaPlayer.notificationVideoSize.connect( sigc::mem_fun(&signalDispatcher, 
				    &SignalDispatcher::on_notification_video_size) );
    mediaPlayer.notificationClipping.connect( sigc::mem_fun(&signalDispatcher, 
				    &SignalDispatcher::on_notification_clipping) );
    mediaPlayer.signal_key_press_event().connect(sigc::mem_fun(signalDispatcher,
				    &SignalDispatcher::on_key_press_event));
    mediaPlayer.notificationFileClosed.connect( sigc::mem_fun(signalDispatcher,
				    &SignalDispatcher::on_file_closed) );

    mediaReceiver.notificationChannelTuned.connect( sigc::mem_fun(&channelConfigWindow, &ChannelConfigWindow::on_tuner_channel_tuned) );
    mediaReceiver.notificationSignalDetected.connect( sigc::mem_fun(&channelConfigWindow, &ChannelConfigWindow::on_tuner_signal_detected) );
    mediaReceiver.notificationScanStopped.connect( sigc::mem_fun(&channelConfigWindow, &ChannelConfigWindow::on_tuner_scan_stopped) );
    mediaReceiver.notificationScanFinished.connect( sigc::mem_fun(&channelConfigWindow, &ChannelConfigWindow::on_tuner_scan_finished) );
    channelConfigWindow.signalSetFrequency.connect( sigc::mem_fun(&mediaReceiver, &GtkmmMediaReceiver::setFrequency) );
    channelConfigWindow.signalStartScan.connect( sigc::mem_fun(&mediaReceiver, &GtkmmMediaReceiver::startFrequencyScan) );
    signalDispatcher.showChannelConfigWindow.connect( sigc::mem_fun(&channelConfigWindow, &ChannelConfigWindow::on_show_window) );

    mediaCommon.signal_configuration_data_loaded.connect(sigc::mem_fun(&channelConfigWindow, &ChannelConfigWindow::on_configuration_data_loaded) );
    channelConfigWindow.signalConfigurationDataChanged.connect(sigc::mem_fun(&mediaCommon, &MediaCommon::saveConfigurationData) );

    mediaCommon.init();

    Gtk::Main::run(mainWindow);

#else

    SyncTestApp syncTestApp;
    syncTestApp();

#endif

    return 0;
}
