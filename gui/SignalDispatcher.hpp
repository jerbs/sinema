//
// Signal Dispatcher
//
// Copyright (C) Joachim Erbs, 2009-2010
//

#ifndef SIGNAL_DISPATCHER_HPP
#define SIGNAL_DISPATCHER_HPP

#include "common/GeneralEvents.hpp"
#include "player/GeneralEvents.hpp"
#include "receiver/GeneralEvents.hpp"
#include "platform/timer.hpp"

#include <boost/shared_ptr.hpp>
#include <gtkmm/action.h>
#include <gtkmm/actiongroup.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/radioaction.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/toggleaction.h>
#include <gtkmm/uimanager.h>

class MainWindow;
class ControlWindow;
class GtkmmPlayList;

class SignalDispatcher
{
    friend class MainWindow;

public:
    sigc::signal<void, bool> showControlWindow;
    sigc::signal<void, bool> showConfigWindow;
    sigc::signal<void, bool> showPlayListWindow;
    sigc::signal<void> hideMainWindow;
    sigc::signal<void, double> zoomMainWindow;
    sigc::signal<void> ignoreWindowResize;

    sigc::signal<void, double> signal_seek_absolute;
    sigc::signal<void, double> signal_seek_relative;
    sigc::signal<void, boost::shared_ptr<ClipVideoSrcEvent> > signal_clip;
    sigc::signal<void> signal_open;
    sigc::signal<void> signal_play;
    sigc::signal<void> signal_pause;
    sigc::signal<void> signal_close;
    sigc::signal<void> signal_skip_forward;
    sigc::signal<void> signal_skip_back;
    sigc::signal<void, double> signal_playback_volume;
    sigc::signal<void, bool> signal_playback_switch;
    sigc::signal<void, const ChannelData&> signalSetFrequency;
    sigc::signal<void, boost::shared_ptr<ConfigurationData> > signalConfigurationDataChanged;

    SignalDispatcher(GtkmmPlayList& playList);
    ~SignalDispatcher();

    Glib::RefPtr<Gtk::UIManager> getUIManager();

    Gtk::Widget* getMenuBarWidget();
    Gtk::Widget* getToolBarWidget();
    Gtk::Menu* getPopupMenuWidget();
    Gtk::Statusbar& getStatusBar();

    Gtk::Widget* getCtrlWinToolBarWidget();

    Glib::RefPtr<Gtk::ActionGroup> getActionGroup();
    Gtk::Adjustment& getPositionAdjustment();
    Gtk::Adjustment& getVolumeAdjustment();

    void setMainWindow(MainWindow* mainWindow);

    // Slots:
    bool on_button_press_event(GdkEventButton* event);
    bool on_key_press_event(GdkEventKey* event);
    void on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context,
			       int x, int y, const Gtk::SelectionData& selection_data,
			       guint info, guint time);
    bool on_config_window_state_event(GdkEventWindowState* event);
    bool on_control_window_state_event(GdkEventWindowState* event);
    bool on_main_window_state_event(GdkEventWindowState* event);
    bool on_play_list_window_state_event(GdkEventWindowState* event);
    void on_notification_video_size(const NotificationVideoSize& event);
    void on_notification_clipping(const NotificationClipping& event);
    void on_notification_file_closed();

    void on_set_title(Glib::ustring title);
    void on_set_duration(double seconds);
    void on_set_time(double seconds);
    void on_set_volume(const NotificationCurrentVolume& vol);

    void on_configuration_data_changed(boost::shared_ptr<ConfigurationData>);
    void on_tuner_channel_tuned(const ChannelData& channelData);

    // Slots:
    virtual void on_file_open();
    virtual void on_file_close();
    virtual void on_file_quit();
    virtual void on_view_fullscreen();
    virtual void on_view_leave_fullscreen();
    virtual void on_view_normal_mode();
    virtual void on_view_tv_mode();
    virtual void on_view_zoom_200();
    virtual void on_view_zoom_100();
    virtual void on_view_zoom_50();
    virtual void on_view_clipping_none();
    virtual void on_view_clipping_custom();
    virtual void on_view_clipping_43();
    virtual void on_view_clipping_169();
    virtual void on_view_controlwindow();
    virtual void on_view_configwindow();
    virtual void on_view_playlistwindow();
    virtual void on_view_menubar();
    virtual void on_view_toolbar();
    virtual void on_view_statusbar();
    virtual void on_media_play();
    virtual void on_media_pause();
    virtual void on_media_stop();
    virtual void on_media_next();
    virtual void on_media_previous();
    virtual void on_media_forward();
    virtual void on_media_rewind();
    virtual void on_media_record();
    virtual void on_channel_next();
    virtual void on_channel_previous();
    virtual void on_channel_selected(int num);
    virtual void on_help_help();
    virtual void on_help_about();
    virtual void on_position_changed();
    virtual void on_position_value_changed();
    virtual void on_volume_changed();
    virtual void on_volume_value_changed();
    virtual void on_mute_toggled();

private:
    void updateGuiConfiguration();

    MainWindow* m_MainWindow;

    GtkmmPlayList& m_PlayList;

    Gtk::UIManager::ui_merge_id m_UiMergeIdChannels;

    Glib::RefPtr<Gtk::UIManager> m_refUIManager;
    Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup;
    Glib::RefPtr<Gtk::ActionGroup> m_refActionGroupChannels;
    Glib::RefPtr<Gtk::Action> m_refActionEnterFullscreen;
    Glib::RefPtr<Gtk::Action> m_refActionLeaveFullscreen;
    Glib::RefPtr<Gtk::Action> m_refActionPlay;
    Glib::RefPtr<Gtk::Action> m_refActionPause;
    Glib::RefPtr<Gtk::RadioAction> m_refClippingNone;
    Glib::RefPtr<Gtk::RadioAction> m_refClippingCustom;
    Glib::RefPtr<Gtk::RadioAction> m_refClipping43;
    Glib::RefPtr<Gtk::RadioAction> m_refClipping169;
    Glib::RefPtr<Gtk::ToggleAction> m_refMute;
    Glib::RefPtr<Gtk::ToggleAction> m_refControlWindowVisible;
    Glib::RefPtr<Gtk::ToggleAction> m_refConfigWindowVisible;
    Glib::RefPtr<Gtk::ToggleAction> m_refPlayListWindowVisible;
    Glib::RefPtr<Gtk::ToggleAction> m_refMenuBarVisible;
    Glib::RefPtr<Gtk::ToggleAction> m_refToolBarVisible;
    Glib::RefPtr<Gtk::ToggleAction> m_refStatusBarVisible;

    ClipVideoSrcEvent getClipVideoSrcEventNone();
    ClipVideoSrcEvent getClipVideoSrcEvent43();
    ClipVideoSrcEvent getClipVideoSrcEvent169();

    bool m_acceptAdjustmentPositionValueChanged;
    bool m_acceptAdjustmentVolumeValueChanged;
    Gtk::Adjustment m_AdjustmentPosition;
    Gtk::Adjustment m_AdjustmentVolume;

    Gtk::Statusbar m_StatusBar;

    bool m_quit;

    NotificationVideoSize m_VideoSize;

    timespec_t m_timeTitlePlaybackStarted;

    ConfigurationGuiVisible m_visibleFullscreen;
    ConfigurationGuiVisible m_visibleWindow;
    ConfigurationGuiVisible* m_visible;
    bool m_fullscreen;

    boost::shared_ptr<ConfigurationData> m_ConfigurationData;
    std::vector<Glib::RefPtr<Gtk::RadioAction> > m_ChannelSelectRadioAction;
    bool m_isEnabled_signalSetFrequency;
    Glib::RefPtr<Gtk::RadioAction> m_refNoChannel;
    int m_tunedFrequency;
};

#endif
