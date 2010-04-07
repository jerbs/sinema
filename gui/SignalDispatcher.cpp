//
// Signal Dispatcher
//
// Copyright (C) Joachim Erbs, 2009, 2010
//

#include <iostream>
#include <gtkmm/menu.h>
#include <gtkmm/stock.h>
#include <gdk/gdkkeysyms.h>

#include "platform/timer.hpp"

#include "gui/ControlWindow.hpp"
#include "gui/MainWindow.hpp"
#include "gui/SignalDispatcher.hpp"

#undef INFO
#define INFO(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

SignalDispatcher::SignalDispatcher()
    : m_MainWindow(0),
      acceptAdjustmentPositionValueChanged(true),
      acceptAdjustmentVolumeValueChanged(true),
      m_AdjustmentPosition(0.0, 0.0, 101.0, 0.1, 1.0, 1.0),
      m_AdjustmentVolume(0.0, 0.0, 101.0, 0.1, 1.0, 1.0),
      timeTitlePlaybackStarted(getTimespec(0)),
      m_visibleFullscreen(false,false,false),
      m_visibleWindow(true,true,true),
      m_visible(&m_visibleWindow),
      m_fullscreen(false)
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

    m_refActionGroup->add(Gtk::Action::create("ViewNormalMode",
					      "Normal Mode", "Show all "),
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_normal_mode));
    m_refActionGroup->add(Gtk::Action::create("ViewTvMode",
					      "TV Mode", "Leave fullscrean mode"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_tv_mode));

    m_refActionGroup->add(Gtk::Action::create("ViewZoom200", "Zoom 200%"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_zoom_200));
    m_refActionGroup->add(Gtk::Action::create("ViewZoom100", "Zoom 100%"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_zoom_100));
    m_refActionGroup->add(Gtk::Action::create("ViewZoom50", "Zoom 50%"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_zoom_50));

    m_refClippingNone = Gtk::RadioAction::create(m_groupClipping, "ViewClippingNone", "No Clipping");
    m_refActionGroup->add(m_refClippingNone,
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_clipping_none) );
    m_refClippingCustom = Gtk::RadioAction::create(m_groupClipping, "ViewClippingCustom", "Custom Clipping");
    m_refActionGroup->add(m_refClippingCustom,
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_clipping_custom) );
    m_refClipping43 = Gtk::RadioAction::create(m_groupClipping, "ViewClipping43", "PAL 4:3");
    m_refActionGroup->add(m_refClipping43,
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_clipping_43) );
    m_refClipping169 = Gtk::RadioAction::create(m_groupClipping, "ViewClipping169", "PAL 16:9");
    m_refActionGroup->add(m_refClipping169,
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_clipping_169) );

    m_refMute = Gtk::ToggleAction::create("ViewMute", "Mute");
    m_refActionGroup->add(m_refMute, sigc::mem_fun(*this, &SignalDispatcher::on_mute_toggled));

    m_refControlWindowVisible = Gtk::ToggleAction::create("ViewControlWindow", "Control Window");
    m_refControlWindowVisible->set_active(false);
    m_refActionGroup->add(m_refControlWindowVisible, sigc::mem_fun(*this, &SignalDispatcher::on_view_controlwindow) );

    m_refChannelConfigWindowVisible = Gtk::ToggleAction::create("ViewChannelConfigWindow", "Channel Config Window");
    m_refChannelConfigWindowVisible->set_active(false);
    m_refActionGroup->add(m_refChannelConfigWindowVisible, sigc::mem_fun(*this, &SignalDispatcher::on_view_channelconfigwindow) );

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
    m_refActionPlay = Gtk::Action::create("MediaPlay", Gtk::Stock::MEDIA_PLAY, "_Play", "Play");
    m_refActionGroup->add(m_refActionPlay, sigc::mem_fun(*this, &SignalDispatcher::on_media_play));
    m_refActionPause = Gtk::Action::create("MediaPause", Gtk::Stock::MEDIA_PAUSE, "_Pause", "Pause");
    m_refActionPause->set_visible(false);
    m_refActionGroup->add(m_refActionPause, sigc::mem_fun(*this, &SignalDispatcher::on_media_pause));
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
        "      <menuitem action='ViewNormalMode'/>"
        "      <menuitem action='ViewTvMode'/>"
        "      <separator/>"
        "      <menuitem action='ViewZoom200'/>"
        "      <menuitem action='ViewZoom100'/>"
        "      <menuitem action='ViewZoom50'/>"
        "      <separator/>"
        "      <menuitem action='ViewClippingNone'/>"
        "      <menuitem action='ViewClippingCustom'/>"
        "      <menuitem action='ViewClipping43'/>"
        "      <menuitem action='ViewClipping169'/>"
        "      <separator/>"
        "      <menuitem action='ViewControlWindow'/>"
        "      <menuitem action='ViewChannelConfigWindow'/>"
        "      <separator/>"
        "      <menuitem action='ViewMute'/>"
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
        "    <menuitem action='ViewNormalMode'/>"
        "    <menuitem action='ViewTvMode'/>"
        "    <separator/>"
        "    <menuitem action='ViewZoom200'/>"
        "    <menuitem action='ViewZoom100'/>"
        "    <menuitem action='ViewZoom50'/>"
        "    <separator/>"
        "    <menuitem action='ViewClippingNone'/>"
        "    <menuitem action='ViewClippingCustom'/>"
        "    <menuitem action='ViewClipping43'/>"
        "    <menuitem action='ViewClipping169'/>"
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

void SignalDispatcher::setMainWindow(MainWindow* mainWindow)
{
    m_MainWindow = mainWindow;
}

bool SignalDispatcher::on_button_press_event(GdkEventButton* event)
{
    if(event->type == GDK_BUTTON_PRESS)
    {
	if (event->button == 2)
	{
	    // Change visibility of ControlWindow:
	    m_refControlWindowVisible->set_active(!m_refControlWindowVisible->get_active());

	    // Event has been handled:
	    return true;
	}
	else if (event->button == 3)
	{
	    Gtk::Menu* popup = getPopupMenuWidget();
	    if (popup)
	    {
		popup->popup(event->button, event->time);
	    }
	    return true;
	}
    }

    // Event has not been handled:
    return false;
}

bool SignalDispatcher::on_key_press_event(GdkEventKey* event)
{
    if(event->type == GDK_KEY_PRESS)
    {
	INFO(<< std::hex << "keyval = " << event->keyval);

	int factor = 1;
	if (event->state & GDK_SHIFT_MASK)
	    factor *= 3;
	if (event->state & GDK_CONTROL_MASK)
	    factor *= 6;

	// Key values are defined in /usr/include/gtk-2.0/gdk/gdkkeysyms.h
	switch (event->keyval)
	{
	case GDK_Left:
	    signal_seek_relative(-10 * factor);
	    break;
	case GDK_Right:
	    signal_seek_relative(+10 * factor);
	    break;
	case ' ':
	    if (m_refActionPlay->is_visible())
		on_media_play();
	    else if (m_refActionPause->is_visible())
		on_media_pause();
	    break;
	case GDK_Up:
	    m_AdjustmentVolume.set_value(m_AdjustmentVolume.get_value() +
					 m_AdjustmentVolume.get_step_increment() * factor);
	    break;
	case GDK_Down:
	    m_AdjustmentVolume.set_value(m_AdjustmentVolume.get_value() -
					 m_AdjustmentVolume.get_step_increment() * factor);
	    break;
	case 'm':
	    m_refMute->set_active(!m_refMute->get_active());
	    break;
	case GDK_Page_Up:
	    on_media_previous();
	    break;
	case GDK_Page_Down:
	    on_media_next();
	    break;
	case GDK_Escape:
	    on_file_quit();
	    break;
	}
    }
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
    if (m_MainWindow)
    {
	m_MainWindow->fullscreen();
    }
}

void SignalDispatcher::on_view_leave_fullscreen()
{
    if (m_MainWindow)
    {
	m_MainWindow->unfullscreen();
    }
}

void SignalDispatcher::on_view_normal_mode()
{
    on_view_leave_fullscreen();

    ignoreWindowResize();

    m_visibleWindow.menuBar = true;
    m_visibleWindow.toolBar = true;
    m_visibleWindow.statusBar = true;

    if (!m_fullscreen)
    {
	m_refMenuBarVisible->set_active(true);
	m_refToolBarVisible->set_active(true);
	m_refStatusBarVisible->set_active(true);
    }
}

void SignalDispatcher::on_view_tv_mode()
{
    on_view_leave_fullscreen();

    ignoreWindowResize();

    m_visibleWindow.menuBar = false;
    m_visibleWindow.toolBar = false;
    m_visibleWindow.statusBar = false;

    if (!m_fullscreen)
    {
	m_refMenuBarVisible->set_active(false);
	m_refToolBarVisible->set_active(false);
	m_refStatusBarVisible->set_active(false);    
    }
}

void SignalDispatcher::on_view_zoom_200()
{
    zoomMainWindow(2);
}

void SignalDispatcher::on_view_zoom_100()
{
    zoomMainWindow(1);
}

void SignalDispatcher::on_view_zoom_50()
{
    zoomMainWindow(0.5);
}

void SignalDispatcher::on_view_clipping_none()
{
    INFO();
    if (m_refClippingNone->get_active())
    {
	signal_clip(boost::make_shared<ClipVideoSrcEvent>(getClipVideoSrcEventNone()));
    }
}

void SignalDispatcher::on_view_clipping_custom()
{
    INFO();
    if (m_refClippingCustom->get_active())
    {

    }
}

ClipVideoSrcEvent SignalDispatcher::getClipVideoSrcEventNone()
{
    // No clipping:
    int l = 0;
    int r = m_VideoSize.widthVid;
    int t = 0;
    int b = m_VideoSize.heightVid;
    return ClipVideoSrcEvent(l,r,t,b);
}

ClipVideoSrcEvent SignalDispatcher::getClipVideoSrcEvent43()
{
    // "PAL 4:3" clipping
    int l = 16;
    int r = m_VideoSize.widthVid - 16;
    int t = 16;
    int b = m_VideoSize.heightVid - 16;
    return ClipVideoSrcEvent(l,r,t,b);
}

ClipVideoSrcEvent SignalDispatcher::getClipVideoSrcEvent169()
{
    // "PAL 16:9" clipping
    int l = 16;
    int r = m_VideoSize.widthVid - 16;
    int t = 72;
    int b = m_VideoSize.heightVid - 80;
    return ClipVideoSrcEvent(l,r,t,b);
}

void SignalDispatcher::on_view_clipping_43()
{
    INFO();
    if (m_refClipping43->get_active())
    {
	signal_clip(boost::make_shared<ClipVideoSrcEvent>(getClipVideoSrcEvent43()));
    }
}

void SignalDispatcher::on_view_clipping_169()
{
    INFO();
    if (m_refClipping169->get_active())
    {
	signal_clip(boost::make_shared<ClipVideoSrcEvent>(getClipVideoSrcEvent169()));
    }
}

bool SignalDispatcher::on_control_window_state_event(GdkEventWindowState* event)
{
}

bool SignalDispatcher::on_main_window_state_event(GdkEventWindowState* event)
{
    // This method is called when fullscreen mode is entered or left. This
    // may be triggered by the application itself or by the window manager.

    if (m_fullscreen = event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)
    {
	m_visible = &m_visibleFullscreen;
    }
    else
    {
	m_visible = &m_visibleWindow;
    }

    m_refMenuBarVisible->set_active(m_visible->menuBar);
    m_refToolBarVisible->set_active(m_visible->toolBar);
    m_refStatusBarVisible->set_active(m_visible->statusBar);

    return false;
}

void SignalDispatcher::on_notification_video_size(const NotificationVideoSize& event)
{
    m_VideoSize = event;
}

bool operator==(const NotificationClipping& rect1, const ClipVideoSrcEvent& rect2)
{
    return ( rect1.left == rect2.left &&
	     rect1.right == rect2.right &&
	     rect1.top == rect2.top &&
	     rect1.bottom == rect2.bottom );
}

void SignalDispatcher::on_notification_clipping(const NotificationClipping& event)
{
    INFO( << "(" << event.left << "," << event.right << "," << event.top << "," << event.bottom << ")" );

    if (event == getClipVideoSrcEventNone())
    {
	m_refClippingNone->set_active(true);
    }
    else if (event == getClipVideoSrcEvent43())
    {
	m_refClipping43->set_active(true);
    }
    else if (event == getClipVideoSrcEvent169())
    {
	m_refClipping169->set_active(true);
    }
    else
    {
	m_refClippingCustom->set_active(true);
    }
}

void SignalDispatcher::on_view_controlwindow()
{
    showControlWindow(m_refControlWindowVisible->get_active());
}

void SignalDispatcher::on_view_channelconfigwindow()
{
    showChannelConfigWindow(m_refChannelConfigWindowVisible->get_active());
}
    
void SignalDispatcher::on_view_menubar()
{
    Gtk::Widget* pMenubar = getMenuBarWidget();
    if(pMenubar)
    {
	ignoreWindowResize();

	if (m_visible->menuBar = m_refMenuBarVisible->get_active())
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
	ignoreWindowResize();

	if (m_visible->toolBar = m_refToolBarVisible->get_active())
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
    ignoreWindowResize();

    if (m_visible->statusBar = m_refStatusBarVisible->get_active())
    {
	m_StatusBar.show();
    }
    else
    {
	m_StatusBar.hide();
    }
}

void SignalDispatcher::on_media_play()
{
    signal_open();
    signal_play();

    // Hide/show buttons and menu entries:
    m_refActionPlay->set_visible(false);
    m_refActionPause->set_visible(true);
}

void SignalDispatcher::on_media_pause()
{
    signal_pause();

    // Hide/show buttons and menu entries:
    m_refActionPause->set_visible(false);
    m_refActionPlay->set_visible(true);
}

void SignalDispatcher::on_media_stop()
{
    signal_close();
}

void SignalDispatcher::on_media_next()
{
    signal_skip_forward();
    timeTitlePlaybackStarted = timer::get_current_time();
}

void SignalDispatcher::on_media_previous()
{
    if ((timer::get_current_time() - timeTitlePlaybackStarted) < getTimespec(1))
    {
	signal_skip_back();
    }
    else
    {
	signal_seek_absolute(0);
    }
    timeTitlePlaybackStarted = timer::get_current_time();
}

void SignalDispatcher::on_media_forward()
{
    signal_seek_relative(+10);
}

void SignalDispatcher::on_media_rewind()
{
    signal_seek_relative(-10);
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
	signal_seek_absolute(m_AdjustmentPosition.get_value());
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
	signal_playback_volume(m_AdjustmentVolume.get_value());
    }
}

void SignalDispatcher::on_mute_toggled()
{
    if (acceptAdjustmentVolumeValueChanged)
    {
	// Mute button toggled
	signal_playback_switch(m_refMute->get_active());
    }
}

void SignalDispatcher::on_file_closed()
{
    // Playing, show play, hide pause buttons/menues:
    m_refActionPlay->set_visible(true);
    m_refActionPause->set_visible(false);
    m_AdjustmentPosition.set_value(0);
}

void SignalDispatcher::on_set_title(Glib::ustring title)
{
    // A new title is opened.
    timeTitlePlaybackStarted = timer::get_current_time();
}

void SignalDispatcher::on_set_time(double seconds)
{
    acceptAdjustmentPositionValueChanged = false;
    m_AdjustmentPosition.set_value(seconds);
    acceptAdjustmentPositionValueChanged = true;

    // Playing, show pause, hide play buttons/menues:
    m_refActionPlay->set_visible(false);
    m_refActionPause->set_visible(true);
}

void SignalDispatcher::on_set_duration(double seconds)
{
    acceptAdjustmentPositionValueChanged = false;
    m_AdjustmentPosition.set_upper(seconds);
    acceptAdjustmentPositionValueChanged = true;
}

void SignalDispatcher::on_set_volume(const NotificationCurrentVolume& vol)
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
