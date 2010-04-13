//
// Signal Dispatcher
//
// Copyright (C) Joachim Erbs, 2009, 2010
//

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <iostream>
#include <list>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/menu.h>
#include <gtkmm/stock.h>
#include <gdk/gdkkeysyms.h>

#include "platform/timer.hpp"
#include "platform/temp_value.hpp"

#include "gui/ControlWindow.hpp"
#include "gui/MainWindow.hpp"
#include "gui/SignalDispatcher.hpp"
#include "player/PlayList.hpp"
#include "receiver/ChannelFrequencyTable.hpp"

SignalDispatcher::SignalDispatcher(PlayList& playList)
    : m_MainWindow(0),
      m_PlayList(playList),
      m_UiMergeIdChannels(0),
      acceptAdjustmentPositionValueChanged(true),
      acceptAdjustmentVolumeValueChanged(true),
      m_AdjustmentPosition(0.0, 0.0, 101.0, 0.1, 1.0, 1.0),
      m_AdjustmentVolume(0.0, 0.0, 101.0, 0.1, 1.0, 1.0),
      timeTitlePlaybackStarted(getTimespec(0)),
      m_visibleFullscreen(false,false,false),
      m_visibleWindow(true,true,true),
      m_visible(&m_visibleWindow),
      m_fullscreen(false),
      m_isEnabled_signalSetFrequency(true)
{
    // Create actions for menus and toolbars:
    m_refActionGroup = Gtk::ActionGroup::create();
    m_refActionGroupChannels = Gtk::ActionGroup::create();

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

    Gtk::RadioAction::Group clippingGroup;
    m_refClippingNone = Gtk::RadioAction::create(clippingGroup, "ViewClippingNone", "No Clipping");
    m_refActionGroup->add(m_refClippingNone,
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_clipping_none) );
    m_refClippingCustom = Gtk::RadioAction::create(clippingGroup, "ViewClippingCustom", "Custom Clipping");
    m_refActionGroup->add(m_refClippingCustom,
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_clipping_custom) );
    m_refClipping43 = Gtk::RadioAction::create(clippingGroup, "ViewClipping43", "PAL 4:3");
    m_refActionGroup->add(m_refClipping43,
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_clipping_43) );
    m_refClipping169 = Gtk::RadioAction::create(clippingGroup, "ViewClipping169", "PAL 16:9");
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

    // Menu: Channels
    m_refActionGroup->add(Gtk::Action::create("ChannelMenu", "Channels"));
    m_refActionGroup->add(Gtk::Action::create("ChannelNext", "_Next", "Switch to next channel"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_channel_next));
    m_refActionGroup->add(Gtk::Action::create("ChannelPrevious", "_Previous", "Switch to previous channel"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_channel_previous));
    // Menu: Help
    m_refActionGroup->add( Gtk::Action::create("HelpMenu", "Help") );
    m_refActionGroup->add( Gtk::Action::create("HelpHelp", Gtk::Stock::HELP),
			   sigc::mem_fun(*this, &SignalDispatcher::on_help_help) );
    m_refActionGroup->add( Gtk::Action::create("HelpAbout", Gtk::Stock::ABOUT),
			   sigc::mem_fun(*this, &SignalDispatcher::on_help_about) );

    m_refUIManager = Gtk::UIManager::create();
    m_refUIManager->insert_action_group(m_refActionGroup);
    m_refUIManager->insert_action_group(m_refActionGroupChannels);

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
	"    <menu action='ChannelMenu'>"
	"      <menuitem action='ViewChannelConfigWindow'/>"
	"      <menuitem action='ChannelPrevious'/>"
	"      <menuitem action='ChannelNext'/>"
        "      <separator/>"
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
	"    <menu action='ChannelMenu'>"
	"      <menuitem action='ViewChannelConfigWindow'/>"
	"      <menuitem action='ChannelNext'/>"
	"      <menuitem action='ChannelPrevious'/>"
        "      <separator/>"
	"    </menu>"
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
	    // on_channel_previous();
	    break;
	case GDK_Page_Down:
	    on_media_next();
	    // on_channel_next();
	    break;
	case GDK_Escape:
	    m_MainWindow->unfullscreen();
	    break;
	}
    }
}

void SignalDispatcher::on_file_open()
{
    DEBUG();

    Gtk::FileChooserDialog dialog("Please choose a file",
				  Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*m_MainWindow);
    dialog.set_select_multiple(true);

    //Add response buttons the the dialog:
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    //Add filters, so that only certain file types can be selected:

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dialog.add_filter(filter_any);

    Gtk::FileFilter filter_video;
    filter_video.set_name("Video files");
    filter_video.add_mime_type("video/*");
    dialog.add_filter(filter_video);

    Gtk::FileFilter filter_audio;
    filter_audio.set_name("Audio files");
    filter_audio.add_mime_type("audio/*");
    dialog.add_filter(filter_audio);

    Gtk::FileFilter filter_image;
    filter_image.set_name("Image files");
    filter_image.add_mime_type("image/*");
    dialog.add_filter(filter_image);

    //Show the dialog and wait for a user response:
    int result = dialog.run();

    //Handle the response:
    switch(result)
    {
    case(Gtk::RESPONSE_OK):
	{
	    DEBUG(<< "RESPONSE_OK");

	    // Get list of selected files:
	    std::list<std::string> filenames = dialog.get_filenames();

	    // Print file names into log file:
	    {
		TraceUnit traceUnit;
		traceUnit << __PRETTY_FUNCTION__ << " selected files:\n"; 
		copy(filenames.begin(), filenames.end(),
		     std::ostream_iterator<std::string>(traceUnit, "\n"));
	    }

	    // Clear existing play list:
	    m_PlayList.clear();

	    // Use address of PlayList, otherwise the object is copied.
	    for_each(filenames.begin(), filenames.end(),
		     boost::bind(&PlayList::append, &m_PlayList, _1));

	    // Start playing the new list:
	    on_media_stop();
	    on_media_play();

	    break;
	}
    case(Gtk::RESPONSE_CANCEL):
	{
	    DEBUG(<< "RESPONSE_CANCEL");
	    break;
	}
    default:
	{
	    DEBUG(<< "result = " << result);
	    break;
	}
    }   
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
    if (m_refClippingNone->get_active())
    {
	signal_clip(boost::make_shared<ClipVideoSrcEvent>(getClipVideoSrcEventNone()));
    }
}

void SignalDispatcher::on_view_clipping_custom()
{
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
    if (m_refClipping43->get_active())
    {
	signal_clip(boost::make_shared<ClipVideoSrcEvent>(getClipVideoSrcEvent43()));
    }
}

void SignalDispatcher::on_view_clipping_169()
{
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
    DEBUG( << "(" << event.left << "," << event.right << "," << event.top << "," << event.bottom << ")" );

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

void SignalDispatcher::on_channel_next()
{
    DEBUG();
    int size = m_ChannelSelectRadioAction.size();
    if (size)
    {
	Glib::RefPtr<Gtk::RadioAction> ra = m_ChannelSelectRadioAction[0];
	int c = ra->get_current_value();
	c++;
	if (c>=size) c = 0;
	ra->set_current_value(c);
    }
}

void SignalDispatcher::on_channel_previous()
{
    DEBUG();
    int size = m_ChannelSelectRadioAction.size();
    if (size)
    {
	Glib::RefPtr<Gtk::RadioAction> ra = m_ChannelSelectRadioAction[0];
	int c = ra->get_current_value();
	c--;
	if (c<0) c = size-1;
	ra->set_current_value(c);
    }
}

void SignalDispatcher::on_channel_selected(int num)
{
    DEBUG(<< "channel = " << num);
    Glib::RefPtr<Gtk::RadioAction> ra = m_ChannelSelectRadioAction[num];
    if (ra && ra->get_active() && m_isEnabled_signalSetFrequency)
    {
	DEBUG(<< "active");
	StationData& sd = m_ConfigurationData.stationList[num];

	ChannelFrequencyTable cft = ChannelFrequencyTable::create(sd.standard.c_str());
	int ch = ChannelFrequencyTable::getChannelNumber(cft, sd.channel.c_str());
	int freq = ChannelFrequencyTable::getChannelFreq(cft, ch);

	ChannelData channelData;
	channelData.standard = sd.standard;
	channelData.channel = sd.channel;
	channelData.frequency = freq;
	channelData.finetune = sd.fine;
	signalSetFrequency(channelData);
    }
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

void SignalDispatcher::on_configuration_data_changed(const ConfigurationData& configurationData)
{
    DEBUG();

    m_ConfigurationData = configurationData;

    // Remove menu entries from UIManager:
    if (m_UiMergeIdChannels)
    {
	m_refUIManager->remove_ui(m_UiMergeIdChannels);
    }

    // Remove old action group from UIManager:
    m_refUIManager->remove_action_group(m_refActionGroupChannels);

    m_ChannelSelectRadioAction.clear();

    // Create a new action goup
    m_refActionGroupChannels = Gtk::ActionGroup::create();

    // Add new channel selection menu entries:
    Gtk::RadioAction::Group channelGroup;

    Glib::ustring ui_info_channels;

    StationList::const_iterator it = m_ConfigurationData.stationList.begin();
    while(it != m_ConfigurationData.stationList.end())
    {
	const StationData& sd = *it;
	int num = m_ChannelSelectRadioAction.size();
	std::stringstream xmlName;
	xmlName << "Channel" << num;
	std::stringstream xmlCode;
	xmlCode << "<menuitem action='" << xmlName.str() << "'/>";
	ui_info_channels.append(xmlCode.str());
	Glib::RefPtr<Gtk::RadioAction> refRadioAction = Gtk::RadioAction::create(channelGroup, xmlName.str(), sd.name);
	refRadioAction->property_value().set_value(num);
	m_refActionGroupChannels->add(refRadioAction, sigc::bind(sigc::mem_fun(this, &SignalDispatcher::on_channel_selected), num) );
	m_ChannelSelectRadioAction.push_back(refRadioAction);

	it++;
    }

    Glib::ustring ui_info;
    ui_info.append("<ui><menubar name='MenuBar'><menu action='ChannelMenu'>");
    ui_info.append(ui_info_channels);
    ui_info.append("</menu></menubar>");
    ui_info.append("<popup name='PopupMenu'><menu action='ChannelMenu'>");
    ui_info.append(ui_info_channels);
    ui_info.append("</menu></popup></ui>");

    m_refUIManager->insert_action_group(m_refActionGroupChannels);

#ifdef GLIBMM_EXCEPTIONS_ENABLED
    try
    {
	m_UiMergeIdChannels = m_refUIManager->add_ui_from_string(ui_info);
    }
    catch(const Glib::Error& ex)
    {
	std::cerr << "building menus failed: " <<  ex.what();
    }
#else
    std::auto_ptr<Glib::Error> ex;
    m_UiMergeIdChannels = m_refUIManager->add_ui_from_string(ui_info, ex);
    if(ex.get())
    {
	std::cerr << "building menus failed: " <<  ex->what();
    }
#endif //GLIBMM_EXCEPTIONS_ENABLED
}

void SignalDispatcher::on_tuner_channel_tuned(const ChannelData& channelData)
{
    DEBUG();

    // Do not generate signalSetFrequency to avoid recursion:
    TemporaryDisable d(m_isEnabled_signalSetFrequency);

    int tunedFreq = channelData.getTunedFrequency();

    int num = 0;
    StationList::const_iterator it = m_ConfigurationData.stationList.begin();
    while(it != m_ConfigurationData.stationList.end())
    {
	const StationData& sd = *it;

	ChannelFrequencyTable cft = ChannelFrequencyTable::create(sd.standard.c_str());
	int ch = ChannelFrequencyTable::getChannelNumber(cft, sd.channel.c_str());
	int freq = ChannelFrequencyTable::getChannelFreq(cft, ch);

	if (tunedFreq == freq + sd.fine)
	{
	    int size = m_ChannelSelectRadioAction.size();
	    if (size)
	    {
		Glib::RefPtr<Gtk::RadioAction> ra = m_ChannelSelectRadioAction[0];

		// Activate tuned channel in the RadioButtonGroup:
		ra->set_current_value(num);
	    }
	}

	num++;
	it++;
    }
}
