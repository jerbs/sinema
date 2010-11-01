//
// Media Player
//
// Copyright (C) Joachim Erbs, 2009-2010
//
//    This file is part of Sinema.
//
//    Sinema is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Sinema is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Sinema.  If not, see <http://www.gnu.org/licenses/>.
//

#include "gui/About.hpp"
#include "gui/ConfigWindow.hpp"
#include "gui/ChannelConfigWidget.hpp"
#include "gui/PlayerConfigWidget.hpp"
#include "gui/ControlWindow.hpp"
#include "gui/GtkmmMediaPlayer.hpp"
#include "gui/GtkmmMediaCommon.hpp"
#include "gui/GtkmmDaemonProxy.hpp"
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
    TRACE_DEBUG(<< "pid = " << getpid());

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
    GtkmmMediaCommon mediaCommon;
    boost::shared_ptr<GtkmmDaemonProxy> daemonProxy(new GtkmmDaemonProxy());
    SignalDispatcher signalDispatcher(playList);
    ControlWindow controlWindow(signalDispatcher);
    ConfigWindow configWindow;
    ChannelConfigWidget channelConfigWidget;
    PlayerConfigWidget playerConfigWidget;
    PlayListWindow playListWindow(playList);
    MainWindow mainWindow(mediaPlayer, signalDispatcher);
    GtkmmMediaRecorder mediaRecorder;
    InhibitScreenSaver inhibitScreenSaver;
    HelpDialog helpDialog;
    AboutDialog aboutDialog;

    controlWindow.set_transient_for(mainWindow);
    configWindow.set_transient_for(mainWindow);
    playListWindow.set_transient_for(mainWindow);

    signalDispatcher.setMainWindow(&mainWindow);

    configWindow.addWidget(channelConfigWidget, "Channels", "Channels");
    configWindow.addWidget(playerConfigWidget, "Player", "Player");

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
    // Signals: GtkmmMediaPlayer -> PlayerConfigWidget
    mediaPlayer.notificationDeinterlacerList.connect( sigc::mem_fun(playerConfigWidget, &PlayerConfigWidget::on_deinterlacer_list) );

    // ---------------------------------------------------------------
    // Signals: DaemonProxy -> ChannelConfigWidget
    daemonProxy->notificationChannelTuned.connect( sigc::mem_fun(&channelConfigWidget, &ChannelConfigWidget::on_tuner_channel_tuned) );
    daemonProxy->notificationSignalDetected.connect( sigc::mem_fun(&channelConfigWidget, &ChannelConfigWidget::on_tuner_signal_detected) );
    daemonProxy->notificationScanStopped.connect( sigc::mem_fun(&channelConfigWidget, &ChannelConfigWidget::on_tuner_scan_stopped) );
    daemonProxy->notificationScanFinished.connect( sigc::mem_fun(&channelConfigWidget, &ChannelConfigWidget::on_tuner_scan_finished) );

    // Signals: ChannelConfigWidget -> DaemonProxy
    channelConfigWidget.signalSetFrequency.connect( sigc::mem_fun(&*daemonProxy, &DaemonProxy::setFrequency) );
    channelConfigWidget.signalStartScan.connect( sigc::mem_fun(&*daemonProxy, &DaemonProxy::startFrequencyScan) );

    // ---------------------------------------------------------------
    // Signals: ChannelConfigWidget -> SignalDispatcher
    channelConfigWidget.signalConfigurationDataChanged.connect(sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_configuration_data_changed) );

    // ---------------------------------------------------------------
    // Signals: PlayerConfigWidget -> MediaPlayer
    playerConfigWidget.signalEnableOptimalPixelFormat.connect(sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::enableOptimalPixelFormat) );
    playerConfigWidget.signalDisableOptimalPixelFormat.connect(sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::disableOptimalPixelFormat) );
    playerConfigWidget.signalEnableXvClipping.connect(sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::enableXvClipping) );
    playerConfigWidget.signalDisableXvClipping.connect(sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::disableXvClipping) );
    playerConfigWidget.signalSelectDeinterlacer.connect(sigc::mem_fun(mediaPlayer, &GtkmmMediaPlayer::selectDeinterlacer) );

    // ---------------------------------------------------------------
    // Signals: ConfigWindow -> SignalDispatcher
    configWindow.signal_window_state_event().connect(sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_config_window_state_event));

    // Signals: SignalDispatcher -> ConfigWindow
    signalDispatcher.showConfigWindow.connect( sigc::mem_fun(&configWindow, &ConfigWindow::on_show_window) );

    // ---------------------------------------------------------------
    // Signals: GtkmmMediaCommon -> ChannelConfigWidget
    mediaCommon.signal_configuration_data_loaded.connect(sigc::mem_fun(&channelConfigWidget, &ChannelConfigWidget::on_configuration_data_loaded) );

    // Signals: ChannelConfigWidget -> GtkmmMediaCommon
    channelConfigWidget.signalConfigurationDataChanged.connect(sigc::mem_fun(&mediaCommon, &MediaCommon::saveConfigurationData) );

    // ---------------------------------------------------------------
    // Signals: GtkmmMediaCommon -> PlayerConfigWidget
    mediaCommon.signal_configuration_data_loaded.connect(sigc::mem_fun(&playerConfigWidget, &PlayerConfigWidget::on_configuration_data_loaded) );

    // Signals: PlayerConfigWidget -> GtkmmMediaCommon
    playerConfigWidget.signalConfigurationDataChanged.connect(sigc::mem_fun(&mediaCommon, &MediaCommon::saveConfigurationData) );

    // ---------------------------------------------------------------
    // Signals: GtkmmMediaCommon -> SignalDispatcher
    mediaCommon.signal_configuration_data_loaded.connect(sigc::mem_fun(signalDispatcher, &SignalDispatcher::on_configuration_data_changed) );

    // Signals: SignalDispatcher -> GtkmmMediaCommon
    signalDispatcher.signalConfigurationDataChanged.connect(sigc::mem_fun(&mediaCommon, &MediaCommon::saveConfigurationData) );

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
    // Signals: SignalDispatcher -> GtkmmDaemonProxy
    signalDispatcher.signalSetFrequency.connect( sigc::mem_fun(&*daemonProxy, &DaemonProxy::setFrequency) );

    // Signals: GtkmmDaemonProxy -> SignalDispatcher
    daemonProxy->notificationChannelTuned.connect( sigc::mem_fun(&signalDispatcher, &SignalDispatcher::on_tuner_channel_tuned) );

    // ---------------------------------------------------------------
    // Signals: MediaPlayer -> InhibitScreenSaver
    mediaPlayer.notificationCurrentTime.connect( sigc::hide_functor<0, sigc::bound_mem_functor0<void, InhibitScreenSaver> >
						 (sigc::mem_fun(inhibitScreenSaver, &InhibitScreenSaver::simulateUserActivity) ) );
    mediaPlayer.signal_realize().connect(sigc::bind(sigc::mem_fun(inhibitScreenSaver, &InhibitScreenSaver::on_realize), &mediaPlayer) );

    // ---------------------------------------------------------------
    // Signals: SignalDispatcher -> AboutDialog, HelpDialog
    signalDispatcher.showHelpDialog.connect(sigc::mem_fun(helpDialog, &HelpDialog::show));
    signalDispatcher.showAboutDialog.connect(sigc::mem_fun(aboutDialog, &AboutDialog::show));

    // ---------------------------------------------------------------
    // Send init events to all threads of all subsystems:
    mediaPlayer.init();
    mediaCommon.init();
    mediaRecorder.init();
    daemonProxy->init();

    mainWindow.show();

    // Enter gtkmm main loop:
    Gtk::Main::run();

    return 0;
}
