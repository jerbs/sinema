//
// Signal Dispatcher
//
// Copyright (C) Joachim Erbs, 2009
//

#include <iostream>
#include <gtkmm/menu.h>
#include <gtkmm/stock.h>

#include "platform/timer.hpp"

#include "gui/GtkmmMediaPlayer.hpp"
#include "gui/SignalDispatcher.hpp"

#undef INFO
#define INFO(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

SignalDispatcher::SignalDispatcher(GtkmmMediaPlayer& mediaPlayer)
    : m_MediaPlayer(mediaPlayer),
      acceptAdjustmentPositionValueChanged(true),
      acceptAdjustmentVolumeValueChanged(true),
      m_AdjustmentPosition(0.0, 0.0, 101.0, 0.1, 1.0, 1.0),
      m_AdjustmentVolume(0.0, 0.0, 101.0, 0.1, 1.0, 1.0),
      timeTitlePlaybackStarted(getTimespec(0))
{
    // Create actions for menus and toolbars:
    m_refActionGroup = Gtk::ActionGroup::create();

    // Menu: File
    m_refActionGroup->add(Gtk::Action::create("FileMenu", "File"));
    m_refActionGroup->add(Gtk::Action::create("FileOpen", Gtk::Stock::OPEN,
					      "Open...", "Open a file"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_file_open));
    m_refActionGroup->add(Gtk::Action::create("FileClose", Gtk::Stock::CLOSE,
					      "Close", "Close current file"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_file_close));
    m_refActionGroup->add(Gtk::Action::create("FileQuit", Gtk::Stock::QUIT),
			  sigc::mem_fun(*this, &SignalDispatcher::on_file_quit));

    // Menu: View
    m_refActionGroup->add(Gtk::Action::create("ViewMenu", "View"));
    m_refActionGroup->add(Gtk::Action::create("ViewFullscreen", Gtk::Stock::FULLSCREEN,
					      "Fullscreen", "Enter fullscrean mode"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_fullscreen));
    m_refActionGroup->add(Gtk::Action::create("ViewLeaveFullscreen", Gtk::Stock::LEAVE_FULLSCREEN,
					      "Leave Fullscreen", "Leave fullscrean mode"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_leave_fullscreen));
    m_refMute = Gtk::ToggleAction::create("ViewMute", "Mute");
    m_refActionGroup->add(m_refMute, sigc::mem_fun(*this, &SignalDispatcher::on_mute_toggled));

    m_refControlWindowVisible = Gtk::ToggleAction::create("ViewControlWindow", "Control Window");
    m_refControlWindowVisible->set_active(true);
    m_refActionGroup->add(m_refControlWindowVisible, sigc::mem_fun(*this, &SignalDispatcher::on_view_controlwindow) );

    m_refMenuBarVisible = Gtk::ToggleAction::create("ViewMenuBar", "Menu Bar");
    m_refMenuBarVisible->set_active(true);
    m_refActionGroup->add(m_refMenuBarVisible, sigc::mem_fun(*this, &SignalDispatcher::on_view_menubar) );

    m_refToolBarVisible = Gtk::ToggleAction::create("ViewToolBar", "Tool Bar");
    m_refToolBarVisible->set_active(true);
    m_refActionGroup->add(m_refToolBarVisible, sigc::mem_fun(*this, &SignalDispatcher::on_view_toolbar) );

    m_refStatusBarVisible = Gtk::ToggleAction::create("ViewStatusBar", "Status Bar");
    m_refStatusBarVisible->set_active(true);
    m_refActionGroup->add(m_refStatusBarVisible, sigc::mem_fun(*this, &SignalDispatcher::on_view_statusbar) );

    // Menu: Media
    m_refActionGroup->add(Gtk::Action::create("MediaMenu", "Media"));
    m_refActionGroup->add(Gtk::Action::create("MediaPlay", Gtk::Stock::MEDIA_PLAY,
					      "_Play", "Play"),
			  Gtk::AccelKey("<shift>p"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_media_play));
    m_refActionGroup->add(Gtk::Action::create("MediaPause", Gtk::Stock::MEDIA_PAUSE,
					      "Pause", "Pause"),
			  Gtk::AccelKey("p"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_media_pause));
    m_refActionGroup->add(Gtk::Action::create("MediaStop", Gtk::Stock::MEDIA_STOP,
					      "_Stop", "Stop"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_media_stop));
    m_refActionGroup->add(Gtk::Action::create("MediaNext", Gtk::Stock::MEDIA_NEXT,
					      "_Next", "Next"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_media_next));
    m_refActionGroup->add(Gtk::Action::create("MediaPrevious", Gtk::Stock::MEDIA_PREVIOUS,
					      "_Previous", "Previous"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_media_previous));
    m_refActionGroup->add(Gtk::Action::create("MediaForward", Gtk::Stock::MEDIA_FORWARD,
					      "_Forward", "Forward"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_media_forward));
    m_refActionGroup->add(Gtk::Action::create("MediaRewind", Gtk::Stock::MEDIA_REWIND,
					      "_Rewind", "Rewind"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_media_rewind));
    m_refActionGroup->add(Gtk::Action::create("MediaRecord", Gtk::Stock::MEDIA_RECORD,
					      "Record", "Record"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_media_record));

    // Menu: Help
    m_refActionGroup->add( Gtk::Action::create("HelpMenu", "Help") );
    m_refActionGroup->add( Gtk::Action::create("HelpHelp", Gtk::Stock::HELP),
			   sigc::mem_fun(*this, &SignalDispatcher::on_help_help) );
    m_refActionGroup->add( Gtk::Action::create("HelpAbout", Gtk::Stock::ABOUT),
			   sigc::mem_fun(*this, &SignalDispatcher::on_help_about) );

    m_refUIManager = Gtk::UIManager::create();
    m_refUIManager->insert_action_group(m_refActionGroup);


    //Layout the actions in a menubar and toolbar:
    Glib::ustring ui_info =
        "<ui>"
        "  <menubar name='MenuBar'>"
        "    <menu action='FileMenu'>"
        "      <menuitem action='FileOpen'/>"
        "      <menuitem action='FileClose'/>"
        "      <menuitem action='FileQuit'/>"
        "    </menu>"
        "    <menu action='ViewMenu'>"
        "      <menuitem action='ViewFullscreen'/>"
        "      <menuitem action='ViewLeaveFullscreen'/>"
        "      <separator/>"
        "      <menuitem action='ViewControlWindow'/>"
        "      <separator/>"
        "      <menuitem action='ViewMenuBar'/>"
        "      <menuitem action='ViewToolBar'/>"
        "      <menuitem action='ViewStatusBar'/>"
        "    </menu>"
        "    <menu action='MediaMenu'>"
        "      <menuitem action='MediaPlay'/>"
        "      <menuitem action='MediaPause'/>"
        "      <menuitem action='MediaStop'/>"
        "      <separator/>"
        "      <menuitem action='MediaNext'/>"
        "      <menuitem action='MediaPrevious'/>"
        "      <menuitem action='MediaForward'/>"
        "      <menuitem action='MediaRewind'/>"
        "      <separator/>"
        "      <menuitem action='MediaRecord'/>"
        "    </menu>"
        "    <menu action='HelpMenu'>"
        "      <menuitem action='HelpHelp'/>"
        "      <menuitem action='HelpAbout'/>"
        "    </menu>"
        "  </menubar>"
        "  <toolbar  name='ToolBar'>"
        "    <toolitem action='FileOpen'/>"
        "    <toolitem action='FileQuit'/>"
        "    <separator/>"
        "    <toolitem action='ViewFullscreen'/>"
        "    <separator/>"
        "    <toolitem action='MediaPrevious'/>"
        "    <toolitem action='MediaRewind'/>"
        "    <toolitem action='MediaPlay'/>"
        "    <toolitem action='MediaPause'/>"
        "    <toolitem action='MediaForward'/>"
        "    <toolitem action='MediaNext'/>"
        "    <toolitem action='MediaStop'/>"
        "    <toolitem action='MediaRecord'/>"
        "  </toolbar>"
        "  <popup name='PopupMenu'>"
        "    <menuitem action='MediaPlay'/>"
        "    <menuitem action='MediaPause'/>"
        "    <separator/>"
        "    <menuitem action='ViewFullscreen'/>"
        "    <menuitem action='ViewLeaveFullscreen'/>"
        "    <separator/>"
        "    <menuitem action='ViewControlWindow'/>"
        "    <separator/>"
        "    <menuitem action='ViewMenuBar'/>"
        "    <menuitem action='ViewToolBar'/>"
        "    <menuitem action='ViewStatusBar'/>"
        "    <separator/>"
        "    <menuitem action='FileQuit'/>"
        "  </popup>"
        "</ui>";

#ifdef GLIBMM_EXCEPTIONS_ENABLED
    try
    {
	m_refUIManager->add_ui_from_string(ui_info);
    }
    catch(const Glib::Error& ex)
    {
	std::cerr << "building menus failed: " <<  ex.what();
    }
#else
    std::auto_ptr<Glib::Error> ex;
    m_refUIManager->add_ui_from_string(ui_info, ex);
    if(ex.get())
    {
	std::cerr << "building menus failed: " <<  ex->what();
    }
#endif //GLIBMM_EXCEPTIONS_ENABLED

    m_AdjustmentPosition.set_lower(0);
    m_AdjustmentPosition.set_upper(0);
    m_AdjustmentPosition.set_page_increment(60);  
    m_AdjustmentPosition.set_page_size(1);
    m_AdjustmentPosition.set_step_increment(10);
    m_AdjustmentPosition.signal_changed().connect(sigc::mem_fun(*this, &SignalDispatcher::on_position_changed));
    m_AdjustmentPosition.signal_value_changed().connect(sigc::mem_fun(*this, &SignalDispatcher::on_position_value_changed));

    m_AdjustmentVolume.set_lower(0);
    m_AdjustmentVolume.set_upper(0);
    m_AdjustmentVolume.set_page_increment(0);  
    m_AdjustmentVolume.set_page_size(0);
    m_AdjustmentVolume.set_step_increment(0);
    m_AdjustmentVolume.signal_changed().connect(sigc::mem_fun(*this, &SignalDispatcher::on_volume_changed));
    m_AdjustmentVolume.signal_value_changed().connect(sigc::mem_fun(*this, &SignalDispatcher::on_volume_value_changed));

    m_MediaPlayer.notificationFileName.connect( sigc::mem_fun(*this, &SignalDispatcher::set_title) );
    m_MediaPlayer.notificationDuration.connect( sigc::mem_fun(*this, &SignalDispatcher::set_duration) );
    m_MediaPlayer.notificationCurrentTime.connect( sigc::mem_fun(*this, &SignalDispatcher::set_time) );
    m_MediaPlayer.notificationCurrentVolume.connect( sigc::mem_fun(*this, &SignalDispatcher::set_volume) );

    m_StatusBar.push("...");
}

SignalDispatcher::~SignalDispatcher()
{
}

Glib::RefPtr<Gtk::UIManager> SignalDispatcher::getUIManager()
{
    return m_refUIManager;
}

Gtk::Widget* SignalDispatcher::getMenuBarWidget()
{
    return m_refUIManager->get_widget("/MenuBar");
}

Gtk::Widget* SignalDispatcher::getToolBarWidget()
{
    return m_refUIManager->get_widget("/ToolBar");
}

Gtk::Menu* SignalDispatcher::getPopupMenuWidget()
{
    return dynamic_cast<Gtk::Menu*>(m_refUIManager->get_widget("/PopupMenu"));
}

Gtk::Statusbar& SignalDispatcher::getStatusBar()
{
    return m_StatusBar;
}

Glib::RefPtr<Gtk::ActionGroup> SignalDispatcher::getActionGroup()
{
    return m_refActionGroup;
}

Gtk::Adjustment& SignalDispatcher::getPositionAdjustment()
{
    return m_AdjustmentPosition;
}

Gtk::Adjustment& SignalDispatcher::getVolumeAdjustment()
{
    return m_AdjustmentVolume;
}

void SignalDispatcher::on_file_open()
{
    INFO();
}

void SignalDispatcher::on_file_close()
{
    INFO();
}

void SignalDispatcher::on_file_quit()
{
    // Hiding the main window stops Gtk::Main::run():
    hideMainWindow();
}

void SignalDispatcher::on_view_fullscreen()
{
    INFO();
}

void SignalDispatcher::on_view_leave_fullscreen()
{
    INFO();
}

void SignalDispatcher::on_view_controlwindow()
{
    showControlWindow(m_refControlWindowVisible->get_active());
}
    
void SignalDispatcher::on_view_menubar()
{
    Gtk::Widget* pMenubar = getMenuBarWidget();
    if(pMenubar)
    {
	if (m_refMenuBarVisible->get_active())
	{
	    pMenubar->show();
	}
	else
	{
	    pMenubar->hide();
	}
    }
}

void SignalDispatcher::on_view_toolbar()
{
    Gtk::Widget* pToolbar = getToolBarWidget();
    if (pToolbar)
    {
	if (m_refToolBarVisible->get_active())
	{
	    pToolbar->show();
	}
	else
	{
	    pToolbar->hide();
	}
    }
}

void SignalDispatcher::on_view_statusbar()
{
    if (m_refStatusBarVisible->get_active())
    {
	INFO(<< "show");
	m_StatusBar.show();
    }
    else
    {
	INFO(<< "hide");
	m_StatusBar.hide();
    }
}

void SignalDispatcher::on_media_play()
{
    m_MediaPlayer.open();
    m_MediaPlayer.play();
    timeTitlePlaybackStarted = timer::get_current_time();
}

void SignalDispatcher::on_media_pause()
{
    m_MediaPlayer.pause();
}

void SignalDispatcher::on_media_stop()
{
    m_MediaPlayer.close();
}

void SignalDispatcher::on_media_next()
{
    m_MediaPlayer.skipForward();
    timeTitlePlaybackStarted = timer::get_current_time();
}

void SignalDispatcher::on_media_previous()
{
    if ((timer::get_current_time() - timeTitlePlaybackStarted) < getTimespec(1))
    {
	m_MediaPlayer.skipBack();
    }
    else
    {
	m_MediaPlayer.seekAbsolute(0);
    }
    timeTitlePlaybackStarted = timer::get_current_time();
}

void SignalDispatcher::on_media_forward()
{
    m_MediaPlayer.seekRelative(+10);
}

void SignalDispatcher::on_media_rewind()
{
    m_MediaPlayer.seekRelative(-10);
}

void SignalDispatcher::on_media_record()
{
    INFO();
}

void SignalDispatcher::on_help_help()
{
    INFO();
}

void SignalDispatcher::on_help_about()
{
    INFO();
}

void SignalDispatcher::on_position_changed()
{
    // Adjustment other than the value changed.
}

void SignalDispatcher::on_position_value_changed()
{
    if (acceptAdjustmentPositionValueChanged)
    {
	// Adjustment value changed.
	m_MediaPlayer.seekAbsolute(m_AdjustmentPosition.get_value());
    }
}

void SignalDispatcher::on_volume_changed()
{
}

void SignalDispatcher::on_volume_value_changed()
{
    if (acceptAdjustmentVolumeValueChanged)
    {
	// Adjustment value changed.
	m_MediaPlayer.setPlaybackVolume(m_AdjustmentVolume.get_value());
    }
}

void SignalDispatcher::on_mute_toggled()
{
    if (acceptAdjustmentVolumeValueChanged)
    {
	// Mute button toggled
	m_MediaPlayer.setPlaybackSwitch(m_refMute->get_active());
    }
}
void SignalDispatcher::set_title(Glib::ustring title)
{
}

void SignalDispatcher::set_time(double seconds)
{
    acceptAdjustmentPositionValueChanged = false;
    m_AdjustmentPosition.set_value(seconds);
    acceptAdjustmentPositionValueChanged = true;
}

void SignalDispatcher::set_duration(double seconds)
{
    acceptAdjustmentPositionValueChanged = false;
    m_AdjustmentPosition.set_upper(seconds);
    acceptAdjustmentPositionValueChanged = true;
}

void SignalDispatcher::set_volume(const NotificationCurrentVolume& vol)
{
    double stepIncrement = double(vol.maxVolume - vol.minVolume) / 100;
    double pageIncrement = stepIncrement * 5;
    acceptAdjustmentVolumeValueChanged = false;
    m_AdjustmentVolume.set_lower(vol.minVolume);
    m_AdjustmentVolume.set_upper(vol.maxVolume);
    m_AdjustmentVolume.set_page_increment(pageIncrement);
    m_AdjustmentVolume.set_step_increment(stepIncrement);
    m_AdjustmentVolume.set_value(vol.volume);
    m_refMute->set_active(vol.enabled);
    acceptAdjustmentVolumeValueChanged = true;
}
