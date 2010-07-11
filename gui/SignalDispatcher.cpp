//
// Signal Dispatcher
//
// Copyright (C) Joachim Erbs, 2009-2010
//

#include "platform/timer.hpp"
#include "platform/temp_value.hpp"

#include "gui/ControlWindow.hpp"
#include "gui/MainWindow.hpp"
#include "gui/SignalDispatcher.hpp"
#include "gui/GtkmmPlayList.hpp"
#include "receiver/ChannelFrequencyTable.hpp"

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <iostream>
#include <list>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/main.h>
#include <gtkmm/menu.h>
#include <gtkmm/stock.h>
#include <gdk/gdkkeysyms.h>

SignalDispatcher::SignalDispatcher(GtkmmPlayList& playList)
    : m_MainWindow(0),
      m_PlayList(playList),
      m_UiMergeIdChannels(0),
      m_acceptAdjustmentPositionValueChanged(true),
      m_acceptAdjustmentVolumeValueChanged(true),
      m_AdjustmentPosition(0.0, 0.0, 101.0, 0.1, 1.0, 1.0),
      m_AdjustmentVolume(0.0, 0.0, 101.0, 0.1, 1.0, 1.0),
      m_quit(false),
      m_timeTitlePlaybackStarted(getTimespec(0)),
      m_visibleFullscreen(false,false,false),
      m_visibleWindow(true,true,true),
      m_visible(&m_visibleWindow),
      m_fullscreen(false),
      m_isEnabled_signalSetFrequency(true),
      m_tunedFrequency(0)
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

    m_refActionEnterFullscreen = Gtk::Action::create("ViewFullscreen", Gtk::Stock::FULLSCREEN,
						"Fullscreen", "Enter fullscrean mode");
    m_refActionGroup->add(m_refActionEnterFullscreen,
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_fullscreen));

    m_refActionLeaveFullscreen = Gtk::Action::create("ViewLeaveFullscreen", Gtk::Stock::LEAVE_FULLSCREEN,
						     "Leave Fullscreen", "Leave fullscrean mode");
    m_refActionLeaveFullscreen->set_visible(false);
    m_refActionGroup->add(m_refActionLeaveFullscreen,
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
    m_refActionGroup->add(m_refControlWindowVisible,
			  Gtk::AccelKey("<control>1"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_controlwindow) );

    m_refPlayListWindowVisible = Gtk::ToggleAction::create("ViewPlayListWindow", "Play List Window");
    m_refPlayListWindowVisible->set_active(false);
    m_refActionGroup->add(m_refPlayListWindowVisible,
			  Gtk::AccelKey("<control>2"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_playlistwindow) );

    m_refConfigWindowVisible = Gtk::ToggleAction::create("ViewConfigWindow", "Config Window");
    m_refConfigWindowVisible->set_active(false);
    m_refActionGroup->add(m_refConfigWindowVisible,
			  Gtk::AccelKey("<control>3"),
			  sigc::mem_fun(*this, &SignalDispatcher::on_view_configwindow) );

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
        "      <menuitem action='ViewPlayListWindow'/>"
        "      <menuitem action='ViewConfigWindow'/>"
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
	"      <menuitem action='ViewConfigWindow'/>"
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
        "    <separator/>"
        "    <toolitem action='MediaRecord'/>"
        "  </toolbar>"
        "  <toolbar  name='CtrlWinToolBar'>"
	"    <toolitem action='MediaPrevious'/>"
        "    <toolitem action='MediaRewind'/>"
        "    <toolitem action='MediaPlay'/>"
        "    <toolitem action='MediaPause'/>"
        "    <toolitem action='MediaForward'/>"
        "    <toolitem action='MediaNext'/>"
        "    <toolitem action='MediaStop'/>"
        "    <separator/>"
        "    <toolitem action='MediaRecord'/>"
        "    <separator/>"
	"    <toolitem action='ViewFullscreen'/>"
        "    <toolitem action='ViewLeaveFullscreen'/>"
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
        "    <menuitem action='ViewPlayListWindow'/>"
        "    <separator/>"
	"    <menu action='ChannelMenu'>"
	"      <menuitem action='ViewConfigWindow'/>"
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

    m_refActionLeaveFullscreen->set_visible(false);
    m_refActionPause->set_visible(false);

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

Gtk::Widget* SignalDispatcher::getCtrlWinToolBarWidget()
{
    return m_refUIManager->get_widget("/CtrlWinToolBar");
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
	TRACE_DEBUG(<< std::hex << "keyval = " << event->keyval);

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

void SignalDispatcher::on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context,
					     int x, int y, const Gtk::SelectionData& selection_data,
					     guint info, guint time)
{
    TRACE_DEBUG();

    if ( (selection_data.get_length() >= 0) &&
	 (selection_data.get_format() == 8) )
    {
	typedef std::vector<Glib::ustring> FileList;
	FileList file_list = selection_data.get_uris();
	if (file_list.size() > 0)
	{
	    FileList::iterator it = file_list.begin();
	    while(it != file_list.end())
	    {
		Glib::ustring file = Glib::filename_from_uri(*it);
		TRACE_DEBUG(<< file);
		PlayList::iterator itPl = m_PlayList.append(file);
		if (it == file_list.begin())
		{
		    // Start playing the first dropped file:
		    m_PlayList.select(itPl);
		    on_media_stop();
		    on_media_play();
		}

		it++;
	    }

	    context->drag_finish(true,   // success
				 false,  // don't delete
				 time);
	    return;
	}
    }

    context->drag_finish(false,  // no success
			 false,
			 time);
}

void SignalDispatcher::on_file_open()
{
    TRACE_DEBUG();

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
	    TRACE_DEBUG(<< "RESPONSE_OK");

	    // Get list of selected files:
	    std::list<std::string> filenames = dialog.get_filenames();

	    // Print file names into log file:
	    {
		TraceUnit traceUnit;
		traceUnit << __PRETTY_FUNCTION__ << " selected files:\n"; 
		copy(filenames.begin(), filenames.end(),
		     std::ostream_iterator<std::string>(traceUnit, "\n"));
	    }

	    // Select end of list, i.e. the next appended file is selected:
	    m_PlayList.selectEndOfList();

	    // Use address of PlayList, otherwise the object is copied.
	    for_each(filenames.begin(), filenames.end(),
		     boost::bind(&GtkmmPlayList::append, &m_PlayList, _1));

	    // Start playing the new list:
	    on_media_stop();
	    on_media_play();

	    break;
	}
    case(Gtk::RESPONSE_CANCEL):
	{
	    TRACE_DEBUG(<< "RESPONSE_CANCEL");
	    break;
	}
    default:
	{
	    TRACE_DEBUG(<< "result = " << result);
	    break;
	}
    }   
}

void SignalDispatcher::on_file_close()
{
    TRACE_DEBUG();

    on_media_stop();

    // Remove current entry from play list and selecet next entry:
    m_PlayList.erase();

    if (m_PlayList.getCurrent().empty())
    {
	// No entry selected, i.e. closed last file.
	
	// Select first file, but do not start playback:
	m_PlayList.selectFirst();
    }
    else
    {
	if (!m_refActionPlay->get_visible())
	{
	    // Play button is not visible, i.e. player is playing.
	    // Start playing the next file:
	    on_media_play();
	}
    }
}

void SignalDispatcher::on_file_quit()
{
    TRACE_DEBUG();

    // Terminate application when close procedure is finished:
    m_quit = true;

    // Explicitly stop playback. This immediately stops audio.
    signal_close();

    // Hiding the main window to give immediate user feedback.
    // This does not stop Gtk::Main::run(), since it is called
    // with a window parameter:
    hideMainWindow();
}

void SignalDispatcher::on_view_fullscreen()
{
    if (m_MainWindow)
    {
	m_MainWindow->fullscreen();
	updateGuiConfiguration();
    }
}

void SignalDispatcher::on_view_leave_fullscreen()
{
    if (m_MainWindow)
    {
	m_MainWindow->unfullscreen();
	updateGuiConfiguration();
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

    updateGuiConfiguration();
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

    updateGuiConfiguration();
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

void on_window_state_event(Glib::RefPtr<Gtk::ToggleAction> action, GdkEventWindowState* event)
{
    TRACE_DEBUG( << "GDK_WINDOW_STATE_WITHDRAWN:  "
		 << (event->changed_mask & GDK_WINDOW_STATE_WITHDRAWN) << ","
		 << (event->new_window_state & GDK_WINDOW_STATE_WITHDRAWN));
    TRACE_DEBUG( << "GDK_WINDOW_STATE_ICONIFIED:  "
		 << (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED) << ","
		 << (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED));
    TRACE_DEBUG( << "GDK_WINDOW_STATE_MAXIMIZED:  "
		 << (event->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) << ","
		 << (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED));
    TRACE_DEBUG( << "GDK_WINDOW_STATE_STICKY:     "
		 << (event->changed_mask & GDK_WINDOW_STATE_STICKY) << ","
		 << (event->new_window_state & GDK_WINDOW_STATE_STICKY));
    TRACE_DEBUG( << "GDK_WINDOW_STATE_FULLSCREEN: "
		 << (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) << ","
		 << (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN));
    TRACE_DEBUG( << "GDK_WINDOW_STATE_ABOVE:      "
		 << (event->changed_mask & GDK_WINDOW_STATE_ABOVE) << ","
		 << (event->new_window_state & GDK_WINDOW_STATE_ABOVE));
    TRACE_DEBUG( << "GDK_WINDOW_STATE_BELOW:      "
		 << (event->changed_mask & GDK_WINDOW_STATE_BELOW) << ","
		 << (event->new_window_state & GDK_WINDOW_STATE_BELOW));

    if (event->changed_mask & GDK_WINDOW_STATE_WITHDRAWN)
    {
	action->set_active((event->new_window_state & GDK_WINDOW_STATE_WITHDRAWN) ? false : true);
    }

    if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED)
    {
	action->set_active((event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) ? false : true);
    }
}

bool SignalDispatcher::on_config_window_state_event(GdkEventWindowState* event)
{
    TRACE_DEBUG();
    on_window_state_event(m_refConfigWindowVisible, event);
    return false;
}

bool SignalDispatcher::on_control_window_state_event(GdkEventWindowState* event)
{
    TRACE_DEBUG();
    on_window_state_event(m_refControlWindowVisible, event);
    return false;
}

bool SignalDispatcher::on_play_list_window_state_event(GdkEventWindowState* event)
{
    TRACE_DEBUG();
    on_window_state_event(m_refPlayListWindowVisible, event);
    return false;
}

bool SignalDispatcher::on_main_window_state_event(GdkEventWindowState* event)
{
    if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)
    {
	// This branch is executed when fullscreen mode is entered or left. This
	// may be triggered by the application itself or by the window manager.

	bool fullscreen;
	if (m_fullscreen = event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)
	{
	    m_visible = &m_visibleFullscreen;
	    fullscreen = true;
	}
	else
	{
	    m_visible = &m_visibleWindow;
	    fullscreen = false;
	}

	m_refActionEnterFullscreen->set_visible(!fullscreen);
	m_refActionLeaveFullscreen->set_visible(fullscreen);

	m_refMenuBarVisible->set_active(m_visible->menuBar);
	m_refToolBarVisible->set_active(m_visible->toolBar);
	m_refStatusBarVisible->set_active(m_visible->statusBar);
    }

    if (event->changed_mask & GDK_WINDOW_STATE_WITHDRAWN)
    {
	if (event->new_window_state & GDK_WINDOW_STATE_WITHDRAWN)
	{
	    // This branch is executed when the main window is closed.

	    TRACE_DEBUG(<< "GDK_WINDOW_STATE_WITHDRAWN");

	    if (! m_quit)
	    {
		// Terminate application when close procedure is finished:
		m_quit = true;

		// Explicitly stop playback. This immediately stops audio.
		signal_close();
	    }
	}
    }

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
    TRACE_DEBUG( << "(" << event.left << "," << event.right << "," << event.top << "," << event.bottom << ")" );

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

void SignalDispatcher::on_view_configwindow()
{
    showConfigWindow(m_refConfigWindowVisible->get_active());
}

void SignalDispatcher::on_view_playlistwindow()
{
    showPlayListWindow(m_refPlayListWindowVisible->get_active());
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

	updateGuiConfiguration();
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

	updateGuiConfiguration();
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

    updateGuiConfiguration();
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
    m_timeTitlePlaybackStarted = timer::get_current_time();
}

void SignalDispatcher::on_media_previous()
{
    if ((timer::get_current_time() - m_timeTitlePlaybackStarted) < getTimespec(1))
    {
	signal_skip_back();
    }
    else
    {
	signal_seek_absolute(0);
    }
    m_timeTitlePlaybackStarted = timer::get_current_time();
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
    TRACE_DEBUG();
}

void SignalDispatcher::on_channel_next()
{
    TRACE_DEBUG();
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
    TRACE_DEBUG();
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
    TRACE_DEBUG(<< "channel = " << num);
    Glib::RefPtr<Gtk::RadioAction> ra = m_ChannelSelectRadioAction[num];

    if (ra && ra->get_active() && m_isEnabled_signalSetFrequency && m_ConfigurationData)
    {
	TRACE_DEBUG(<< "active");
	StationData& sd = m_ConfigurationData->stationList[num];

	ChannelFrequencyTable cft = ChannelFrequencyTable::create(sd.standard.c_str());
	int ch = ChannelFrequencyTable::getChannelNumber(cft, sd.channel.c_str());
	int freq = ChannelFrequencyTable::getChannelFreq(cft, ch);

	ChannelData channelData;
	channelData.standard = sd.standard;
	channelData.channel = sd.channel;
	channelData.frequency = freq;
	channelData.finetune = sd.fine;
	signalSetFrequency(channelData);

	std::string device = "pvr:/dev/video0";
	if (m_PlayList.getCurrent() != device)
	{
	    if (!m_PlayList.select(device))
	    {
		m_PlayList.append(device);
	    }
	    on_media_stop();
	}
	on_media_play();
    }
}

void SignalDispatcher::on_help_help()
{
    TRACE_DEBUG();
}

void SignalDispatcher::on_help_about()
{
    TRACE_DEBUG();
}

void SignalDispatcher::on_position_changed()
{
    // Adjustment other than the value changed.
}

void SignalDispatcher::on_position_value_changed()
{
    if (m_acceptAdjustmentPositionValueChanged)
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
    if (m_acceptAdjustmentVolumeValueChanged)
    {
	// Adjustment value changed.
	signal_playback_volume(m_AdjustmentVolume.get_value());
    }
}

void SignalDispatcher::on_mute_toggled()
{
    if (m_acceptAdjustmentVolumeValueChanged)
    {
	// Mute button toggled
	signal_playback_switch(m_refMute->get_active());
    }
}

void SignalDispatcher::on_notification_file_closed()
{
    TRACE_DEBUG();

    // MediaPlayer finished playing a file.

    // Not playing, show play, hide pause buttons/menues:
    m_refActionPlay->set_visible(true);
    m_refActionPause->set_visible(false);
    m_AdjustmentPosition.set_value(0);
        
    // MediaPlayer automatically starts playing the next file.

    if (m_PlayList.getCurrent().empty())
    {
	// MediaPlayer finished playing the last file of the play list.

	// Select first file, but do not start playback:
	m_PlayList.selectFirst();
    }

    if (m_quit)
    {
	// Makes the innermost invocation of the main loop return 
	// when it regains control:
	Gtk::Main::quit();
    }
}

void SignalDispatcher::on_set_title(Glib::ustring title)
{
    // A new title is opened.
    m_timeTitlePlaybackStarted = timer::get_current_time();
}

void SignalDispatcher::on_set_time(double seconds)
{
    m_acceptAdjustmentPositionValueChanged = false;
    m_AdjustmentPosition.set_value(seconds);
    m_acceptAdjustmentPositionValueChanged = true;

    // Playing, show pause, hide play buttons/menues:
    m_refActionPlay->set_visible(false);
    m_refActionPause->set_visible(true);
}

void SignalDispatcher::on_set_duration(double seconds)
{
    m_acceptAdjustmentPositionValueChanged = false;
    m_AdjustmentPosition.set_upper(seconds);
    m_acceptAdjustmentPositionValueChanged = true;
}

void SignalDispatcher::on_set_volume(const NotificationCurrentVolume& vol)
{
    double stepIncrement = double(vol.maxVolume - vol.minVolume) / 100;
    double pageIncrement = stepIncrement * 5;
    m_acceptAdjustmentVolumeValueChanged = false;
    m_AdjustmentVolume.set_lower(vol.minVolume);
    m_AdjustmentVolume.set_upper(vol.maxVolume);
    m_AdjustmentVolume.set_page_increment(pageIncrement);
    m_AdjustmentVolume.set_step_increment(stepIncrement);
    m_AdjustmentVolume.set_value(vol.volume);
    m_refMute->set_active(vol.enabled);
    m_acceptAdjustmentVolumeValueChanged = true;
}

int getTunerFrequency(const StationData& sd)
{
    ChannelFrequencyTable cft = ChannelFrequencyTable::create(sd.standard.c_str());
    int ch = ChannelFrequencyTable::getChannelNumber(cft, sd.channel.c_str());
    return ChannelFrequencyTable::getChannelFreq(cft, ch) + sd.fine;
}

void SignalDispatcher::on_configuration_data_changed(boost::shared_ptr<ConfigurationData> event)
{
    TRACE_DEBUG();

    const ConfigurationData& configurationData = *event;

    m_ConfigurationData = event;

    // -------------------------------------------
    // Updating visibility mode:
    m_fullscreen = configurationData.configGui.fullscreen;
    m_visibleWindow = configurationData.configGui.visibleWindow;
    m_visibleFullscreen = configurationData.configGui.visibleFullscreen;
    if (m_fullscreen)
    {
	on_view_fullscreen();
    }
    else
    {
	on_view_leave_fullscreen();
    }

    m_refMenuBarVisible->set_active(m_visible->menuBar);
    m_refToolBarVisible->set_active(m_visible->toolBar);
    m_refStatusBarVisible->set_active(m_visible->statusBar);

    // -------------------------------------------
    // Updating Channel menu:

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

    // Add invisible element to radio button group as first element.
    // It is enabled by default, i.e. none of the visible buttons is enabled.
    // This indicates that none of the configured channels is tuned.
    m_refNoChannel = Gtk::RadioAction::create(channelGroup, "ChannelNone", "No Channel");
    m_refNoChannel->property_value().set_value(-1);

    Glib::ustring ui_info_channels;

    StationList::const_iterator it = m_ConfigurationData->stationList.begin();
    while(it != m_ConfigurationData->stationList.end())
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

	if (m_tunedFrequency == getTunerFrequency(sd))
	{
	    // This is the deferred activation of the RadioAction.
	    // m_tunedFrequency was stored in on_tuner_channel_tuned.
	    TemporaryDisable d(m_isEnabled_signalSetFrequency);
	    refRadioAction->set_current_value(num);
	}

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
    TRACE_DEBUG();

    // This slot may be triggered before the channel list is available.
    // In that case the station list is still empty and setting the
    // RadioAction has to be deferred. The deferred activation of the
    // RadioAction is done in on_configuration_data_changed.

    // Do not generate signalSetFrequency to avoid recursion:
    TemporaryDisable d(m_isEnabled_signalSetFrequency);

    m_tunedFrequency = channelData.getTunedFrequency();

    if (m_ConfigurationData)
    {
	int num = 0;
	StationList::const_iterator it = m_ConfigurationData->stationList.begin();
	while(it != m_ConfigurationData->stationList.end())
	{
	    const StationData& sd = *it;
	    if (m_tunedFrequency == getTunerFrequency(sd))
	    {
		int size = m_ChannelSelectRadioAction.size();
		if (size)
		{
		    Glib::RefPtr<Gtk::RadioAction> ra = m_ChannelSelectRadioAction[0];
		    
		    // Activate tuned channel in the RadioButtonGroup:
		    ra->set_current_value(num);
		}
		return;
	    }

	    num++;
	    it++;
	}
    }

    if (m_refNoChannel)
    {
	// Select the invisible radio action:
	m_refNoChannel->set_current_value(-1);
    }
}

void SignalDispatcher::updateGuiConfiguration()
{
    TRACE_DEBUG();

    if (m_ConfigurationData)
    {
	bool modified = false;

	if (m_ConfigurationData->configGui.fullscreen != m_fullscreen)
	{
	    m_ConfigurationData->configGui.fullscreen = m_fullscreen;
	    modified = true;
	}

	if ( (m_ConfigurationData->configGui.visibleWindow) != m_visibleWindow)
	{
	    m_ConfigurationData->configGui.visibleWindow = m_visibleWindow;
	    modified = true;
	}

	if (m_ConfigurationData->configGui.visibleFullscreen != m_visibleFullscreen)
	{
	    m_ConfigurationData->configGui.visibleFullscreen = m_visibleFullscreen;
	    modified = true;
	}

	if (modified)
	{
	    signalConfigurationDataChanged(m_ConfigurationData);
	}
    }
}
