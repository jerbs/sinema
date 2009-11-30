//
// Signal Dispatcher
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef SIGNAL_DISPATCHER_HPP
#define SIGNAL_DISPATCHER_HPP

#include <gtkmm/action.h>
#include <gtkmm/actiongroup.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/radioaction.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/toggleaction.h>
#include <gtkmm/uimanager.h>

class GtkmmMediaPlayer;
class MainWindow;
class ControlWindow;

class SignalDispatcher
{
public:
    sigc::signal<void, bool> showControlWindow;
    sigc::signal<void> hideMainWindow;
    sigc::signal<void, double> zoomMainWindow;

    SignalDispatcher(GtkmmMediaPlayer& mediaPlayer);
    ~SignalDispatcher();

    Glib::RefPtr<Gtk::UIManager> getUIManager();

    Gtk::Widget* getMenuBarWidget();
    Gtk::Widget* getToolBarWidget();
    Gtk::Menu* getPopupMenuWidget();
    Gtk::Statusbar& getStatusBar();

    Glib::RefPtr<Gtk::ActionGroup> getActionGroup();
    Gtk::Adjustment& getPositionAdjustment();
    Gtk::Adjustment& getVolumeAdjustment();

    void setMainWindow(MainWindow* mainWindow);

    // Slots:
    bool on_button_press_event(GdkEventButton* event);
    bool on_control_window_state_event(GdkEventWindowState* event);
    bool on_main_window_state_event(GdkEventWindowState* event);

private:
    // Slots:
    virtual void on_file_open();
    virtual void on_file_close();
    virtual void on_file_quit();
    virtual void on_view_fullscreen();
    virtual void on_view_leave_fullscreen();
    virtual void on_view_zoom_200();
    virtual void on_view_zoom_100();
    virtual void on_view_zoom_50();
    virtual void on_view_controlwindow();
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
    virtual void on_help_help();
    virtual void on_help_about();
    virtual void on_position_changed();
    virtual void on_position_value_changed();
    virtual void on_volume_changed();
    virtual void on_volume_value_changed();
    virtual void on_mute_toggled();

    void set_title(Glib::ustring title);
    void set_duration(double seconds);
    void set_time(double seconds);
    void set_volume(const NotificationCurrentVolume& vol);

    MainWindow* m_MainWindow;
    GtkmmMediaPlayer& m_MediaPlayer;

    Glib::RefPtr<Gtk::UIManager> m_refUIManager;
    Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup;
    Glib::RefPtr<Gtk::RadioAction> m_refChoiceOne, m_refChoiceTwo;
    Glib::RefPtr<Gtk::ToggleAction> m_refMute;
    Glib::RefPtr<Gtk::ToggleAction> m_refControlWindowVisible;
    Glib::RefPtr<Gtk::ToggleAction> m_refMenuBarVisible;
    Glib::RefPtr<Gtk::ToggleAction> m_refToolBarVisible;
    Glib::RefPtr<Gtk::ToggleAction> m_refStatusBarVisible;

    bool acceptAdjustmentPositionValueChanged;
    bool acceptAdjustmentVolumeValueChanged;
    Gtk::Adjustment m_AdjustmentPosition;
    Gtk::Adjustment m_AdjustmentVolume;

    Gtk::Statusbar m_StatusBar;

    timespec_t timeTitlePlaybackStarted;

    struct Visible {
	Visible(bool menuBar, bool toolBar, bool statusBar)
	    : menuBar(menuBar),
	      toolBar(toolBar),
	      statusBar(statusBar)
	{}
	bool menuBar;
	bool toolBar;
	bool statusBar;
    };

    Visible m_visibleFullscreen;
    Visible m_visibleWindow;
    Visible* m_visible;
    bool m_fullscreen;
};

#endif
