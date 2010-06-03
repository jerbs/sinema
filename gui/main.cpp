//
// Media Player
//
// Copyright (C) Joachim Erbs, 2009, 2010
//

#include "gui/ChannelConfigWindow.hpp"
#include "gui/ControlWindow.hpp"
#include "gui/GtkmmMediaPlayer.hpp"
#include "gui/GtkmmMediaReceiver.hpp"
#include "gui/GtkmmMediaCommon.hpp"
#include "gui/InhibitScreenSaver.hpp"
#include "gui/MainWindow.hpp"
#include "gui/PlayListWindow.hpp"
#include "gui/SignalDispatcher.hpp"
#include "gui/GtkmmMediaRecorder.hpp"

#include <iostream>
#include <gtkmm/main.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{
    DEBUG(<< "pid = " << getpid());

    Gtk::Main kit(argc, argv);

    if (argc < 1)
    {
	std::cout << "Usage: " << argv[0] << " [<file1> <file2> ... <filen>]" << std::endl;
	exit(-1);
    }

    GtkmmPlayList playList;
    for (int i=1; i<argc; i++)
	playList.append(std::string(argv[i]));

    // ---------------------------------------------------------------
    // Constructing the top level objects:

    GtkmmMediaPlayer mediaPlayer(playList);
    GtkmmMediaReceiver mediaReceiver;
    GtkmmMediaCommon mediaCommon;
    SignalDispatcher signalDispatcher(playList);
    ControlWindow controlWindow(signalDispatcher);
    ChannelConfigWindow channelConfigWindow;
    PlayListWindow playListWindow(playList);
    MainWindow mainWindow(mediaPlayer, signalDispatcher);
    GtkmmMediaRecorder mediaRecorder;
    InhibitScreenSaver inhibitScreenSaver;

    controlWindow.set_transient_for(mainWindow);
    channelConfigWindow.set_transient_for(mainWindow);
    playListWindow.set_transient_for(mainWindow);

    signalDispatcher.setMainWindow(&mainWindow);

    // Compiler needs help to find the correct overloaded method:
    void (GtkmmMediaPlayer::*GtkmmMediaPlayer_ClipSrc)(boost::shared_ptr<ClipVideoSrcEvent> event) = &GtkmmMediaPlayer::clip;

    // ---------------------------------------------------------------
    // Signals: GtkmmMediaRecorder -> *
    mediaRecorder.notificationDuration.connect( sigc::mem_fun(controlWindow, &ControlWindow::on_set_duration) );
    mediaRecorder.notificationDuration.connect( sigc::mem_fun(&mainWindow, &MainWindow::set_duration) );
    mediaRecorder.notificationDuration.connect( sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_set_duration) );

    // ---------------------------------------------------------------
    // Signals: GtkmmMediaPlayer -> ControlWindow
    mediaPlayer.notificationFileName.connect( sigc::mem_fun(controlWindow, &ControlWindow::on_set_title) );
    mediaPlayer.notificationDuration.connect( sigc::mem_fun(controlWindow, &ControlWindow::on_set_duration) );
    mediaPlayer.notificationCurrentTime.connect( sigc::mem_fun(controlWindow, &ControlWindow::on_set_time) );

    // ---------------------------------------------------------------
    // Signals: GtkmmMediaPlayer -> MainWindow
    mediaPlayer.notificationVideoSize.connect( sigc::mem_fun(&mainWindow, &MainWindow::on_notification_video_size) );
    mediaPlayer.notificationDuration.connect( sigc::mem_fun(&mainWindow, &MainWindow::set_duration) );
    mediaPlayer.notificationCurrentTime.connect( sigc::mem_fun(&mainWindow, &MainWindow::set_time) );
    mediaPlayer.notificationFileName.connect( sigc::mem_fun(&mainWindow, &MainWindow::set_title) );

    // ---------------------------------------------------------------
    // Signals: GtkmmMediaPlayer -> SignalDispatcher
    mediaPlayer.notificationFileName.connect( sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_set_title) );
    mediaPlayer.notificationDuration.connect( sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_set_duration) );
    mediaPlayer.notificationCurrentTime.connect( sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_set_time) );
    mediaPlayer.notificationCurrentVolume.connect( sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_set_volume) );
    mediaPlayer.notificationVideoSize.connect( sigc::mem_fun(&signalDispatcher, &SignalDispatcher::on_notification_video_size) );
    mediaPlayer.notificationClipping.connect( sigc::mem_fun(&signalDispatcher, &SignalDispatcher::on_notification_clipping) );
    mediaPlayer.signal_key_press_event().connect(sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_key_press_event));
    mediaPlayer.notificationFileClosed.connect( sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_notification_file_closed) );
    mediaPlayer.signal_drag_data_received().connect(sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_drag_data_received));

    // Signals: SignalDispatcher -> GtkmmMediaPlayer
    signalDispatcher.signal_seek_absolute.connect( sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::seekAbsolute) );
    signalDispatcher.signal_seek_relative.connect( sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::seekRelative) );
    signalDispatcher.signal_clip.connect( sigc::mem_fun(mediaPlayer, GtkmmMediaPlayer_ClipSrc ) );
    signalDispatcher.signal_open.connect( sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::open) );
    signalDispatcher.signal_play.connect( sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::play) );
    signalDispatcher.signal_pause.connect( sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::pause) );
    signalDispatcher.signal_close.connect( sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::close) );
    signalDispatcher.signal_skip_forward.connect( sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::skipForward) );
    signalDispatcher.signal_skip_back.connect( sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::skipBack) );
    signalDispatcher.signal_playback_volume.connect( sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::setPlaybackVolume) );
    signalDispatcher.signal_playback_switch.connect( sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::setPlaybackSwitch) );

    // ---------------------------------------------------------------
    // Signals: ControlWindow -> SignalDispatcher
    controlWindow.signal_window_state_event().connect(sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_control_window_state_event));

    // Signals: SignalDispatcher -> ControlWindow
    signalDispatcher.showControlWindow.connect( sigc::mem_fun(controlWindow, &ControlWindow::on_show_window) );

    // ---------------------------------------------------------------
    // Signals: MainWindow -> SignalDispatcher
    mainWindow.signal_button_press_event().connect(sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_button_press_event));
    mainWindow.signal_window_state_event().connect(sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_main_window_state_event));

    // Signals: SignalDispatcher -> MainWindow
    signalDispatcher.zoomMainWindow.connect(sigc::mem_fun(mainWindow, &MainWindow::zoom));
    signalDispatcher.ignoreWindowResize.connect(sigc::mem_fun(mainWindow, &MainWindow::ignoreWindowResize));

    // ---------------------------------------------------------------
    // Signals: GtkmmMediaReceiver -> ChannelConfigWindow
    mediaReceiver.notificationChannelTuned.connect( sigc::mem_fun(&channelConfigWindow, &ChannelConfigWindow::on_tuner_channel_tuned) );
    mediaReceiver.notificationSignalDetected.connect( sigc::mem_fun(&channelConfigWindow, &ChannelConfigWindow::on_tuner_signal_detected) );
    mediaReceiver.notificationScanStopped.connect( sigc::mem_fun(&channelConfigWindow, &ChannelConfigWindow::on_tuner_scan_stopped) );
    mediaReceiver.notificationScanFinished.connect( sigc::mem_fun(&channelConfigWindow, &ChannelConfigWindow::on_tuner_scan_finished) );

    // Signals: ChannelConfigWindow -> GtkmmMediaReceiver
    channelConfigWindow.signalSetFrequency.connect( sigc::mem_fun(&mediaReceiver, &GtkmmMediaReceiver::setFrequency) );
    channelConfigWindow.signalStartScan.connect( sigc::mem_fun(&mediaReceiver, &GtkmmMediaReceiver::startFrequencyScan) );

    // ---------------------------------------------------------------
    // Signals: ChannelConfigWindow -> SignalDispatcher
    channelConfigWindow.signal_window_state_event().connect(sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_channel_config_window_state_event));
    channelConfigWindow.signalConfigurationDataChanged.connect(sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_configuration_data_changed) );

    // Signals: SignalDispatcher -> ChannelConfigWindow
    signalDispatcher.showChannelConfigWindow.connect( sigc::mem_fun(&channelConfigWindow, &ChannelConfigWindow::on_show_window) );

    // ---------------------------------------------------------------
    // Signals: GtkmmMediaCommon -> ChannelConfigWindow
    mediaCommon.signal_configuration_data_loaded.connect(sigc::mem_fun(&channelConfigWindow, &ChannelConfigWindow::on_configuration_data_loaded) );

    // Signals: ChannelConfigWindow -> GtkmmMediaCommon
    channelConfigWindow.signalConfigurationDataChanged.connect(sigc::mem_fun(&mediaCommon, &MediaCommon::saveConfigurationData) );

    // ---------------------------------------------------------------
    // Signals: GtkmmMediaCommon -> SignalDispatcher
    mediaCommon.signal_configuration_data_loaded.connect(sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_configuration_data_changed) );

    // ---------------------------------------------------------------
    // Signals: PlayListWindow -> SignalDispatcher
    playListWindow.signal_window_state_event().connect(sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_play_list_window_state_event));
    playListWindow.signal_file_close.connect(sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_file_close));

    // Signals: SignalDispatcher -> PlayListWindow
    signalDispatcher.showPlayListWindow.connect( sigc::mem_fun(&playListWindow, &PlayListWindow::on_show_window) );

    // ---------------------------------------------------------------
    // Signals: PlayListWindow -> GtkmmMediaPlayer
    playListWindow.signal_open.connect( sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::open) );
    playListWindow.signal_play.connect( sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::play) );
    playListWindow.signal_close.connect( sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::close) );

    // ---------------------------------------------------------------
    // Signals: SignalDispatcher -> GtkmmMediaReceiver
    signalDispatcher.signalSetFrequency.connect( sigc::mem_fun(&mediaReceiver, &GtkmmMediaReceiver::setFrequency) );

    // Signals: GtkmmMediaReceiver -> SignalDispatcher
    mediaReceiver.notificationChannelTuned.connect( sigc::mem_fun(&signalDispatcher, &SignalDispatcher::on_tuner_channel_tuned) );

    // ---------------------------------------------------------------
    // Signals: MediaPlayer -> InhibitScreenSaver
    mediaPlayer.notificationCurrentTime.connect( sigc::hide_functor<0, sigc::bound_mem_functor0<void, InhibitScreenSaver> >
						 (sigc::mem_fun(inhibitScreenSaver, &InhibitScreenSaver::simulateUserActivity) ) );
    mediaPlayer.signal_realize().connect(sigc::bind(sigc::mem_fun(inhibitScreenSaver, &InhibitScreenSaver::on_realize), &mediaPlayer) );

    // ---------------------------------------------------------------
    // Send init events to all threads of all subsystems:
    mediaPlayer.init();
    mediaReceiver.init();
    mediaCommon.init();
    mediaRecorder.init();

    mainWindow.show();

    // Enter gtkmm main loop:
    Gtk::Main::run();

    return 0;
}
