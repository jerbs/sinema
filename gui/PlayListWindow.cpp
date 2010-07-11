//
// Play List Window
//
// Copyright (C) Joachim Erbs, 2010
//

#include "gui/PlayListWindow.hpp"
#include "platform/Logging.hpp"

extern std::string applicationName;

PlayListWindow::PlayListWindow(GtkmmPlayList& playList)
    : m_PlayList(playList),
      m_shown(false),
      m_pos_x(0),
      m_pos_y(0)
{
    set_title(applicationName + " (Play List)");
    set_default_size(400,200);


    // Drop destination:
    std::list<Gtk::TargetEntry> targetList;
    targetList.push_back(Gtk::TargetEntry("text/uri-list"));
    drag_dest_set(targetList);

    m_ScrolledWindow.add(m_TreeView);
    m_Box.pack_start(m_ScrolledWindow, Gtk::PACK_EXPAND_WIDGET);
    add(m_Box);

    // Show scrollbars only when necessary:
    m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    // Create the Tree model:
    m_refTreeModel = PlayListTreeModel::create(playList);
    m_TreeView.set_model(m_refTreeModel);

    // Setup and add column to tree model. The own cell renderer with callback
    // formats the currently played file with a different style:
    Gtk::TreeViewColumn::SlotCellData slot = sigc::mem_fun(*this, &PlayListWindow::prepareCellRenderer);
    Gtk::TreeView::Column* pColumnName = Gtk::manage( new Gtk::TreeView::Column("Name") ); 
    pColumnName->pack_start(m_CellRendererText);
    pColumnName->add_attribute(m_CellRendererText.property_text(), m_Columns.m_col_name);
    pColumnName->set_cell_data_func(m_CellRendererText, slot);
    m_TreeView.append_column(*pColumnName);

    // Allow rows to be drag and dropped within the treeview:
    m_TreeView.set_reorderable();

    // User can type in text to search through the tree interactively ("typeahead find"):
    m_TreeView.set_enable_search(true);

    // Hide table header:    
    m_TreeView.set_headers_visible(false);

    // Context Menu
    // Create actions for menus and toolbars:
    m_refActionGroup = Gtk::ActionGroup::create();
    m_refActionGroup->add(Gtk::Action::create("PlayEntry", "Play Entry"),
                          sigc::mem_fun(*this, &PlayListWindow::on_play_entry));
    m_refActionGroup->add(Gtk::Action::create("RemoveEntry", "Remove Entry"),
                          sigc::mem_fun(*this, &PlayListWindow::on_remove_entry));
    m_refActionGroup->add(Gtk::Action::create("ClearAllEntries", "Clear All Entries"),
                          sigc::mem_fun(*this, &PlayListWindow::on_clear_all_entries));

    m_refUIManager = Gtk::UIManager::create();
    m_refUIManager->insert_action_group(m_refActionGroup);
    m_TreeView.add_events(Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);
    m_ScrolledWindow.add_events(Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);
    add_events(Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);

    Glib::ustring ui_info =
        "<ui>"
        "  <popup name='PopupMenu'>"
        "    <menuitem action='PlayEntry'/>"
        "    <menuitem action='RemoveEntry'/>"
        "    <menuitem action='ClearAllEntries'/>"
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

    // This only works with false. I.e. ChannelConfigWindow::on_button_press_event
    // gets the button press event before the default handler gets the event.
    m_TreeView.signal_button_press_event().connect(sigc::mem_fun(this, &PlayListWindow::on_button_press_event), false);
    m_TreeView.signal_row_activated().connect(sigc::mem_fun(this, &PlayListWindow::on_row_activated), false);

    m_refTreeModel->signal_row_changed().connect(sigc::mem_fun(this, &PlayListWindow::on_row_changed));
    m_refTreeModel->signal_row_inserted().connect(sigc::mem_fun(this, &PlayListWindow::on_row_inserted));
    m_refTreeModel->signal_row_deleted().connect(sigc::mem_fun(this, &PlayListWindow::on_row_deleted));
    m_refTreeModel->signal_rows_reordered().connect(sigc::mem_fun(this, &PlayListWindow::on_rows_reordered));

    show_all_children();
}

PlayListWindow::~PlayListWindow()
{
}

void PlayListWindow::on_show_window(bool pshow)
{
    TRACE_DEBUG();

    // This is the same as ControlWindow::on_show_control_window.

    if (pshow)
    {
        // Let the window manager decide where to place the window when shown
        // for the first time. Use previous position when shown again.
        if (m_shown)
        {
            // The window manager may ignore or modify the move request.
            // Partially visible windows are for example replaced by KDE.
            move(m_pos_x, m_pos_y);
        }
        show();
        raise();
        deiconify();
        m_shown = true;
    }
    else
    {
        get_position(m_pos_x, m_pos_y);
        hide();
    }
}

void PlayListWindow::on_row_activated (const Gtk::TreeModel::Path& path,
				       Gtk::TreeViewColumn* column)
{
    TRACE_DEBUG();

    Gtk::TreeModel::iterator iter = m_refTreeModel->get_iter(path);
    if (iter)
    {
	playEntry(iter);
    }
}

bool PlayListWindow::on_button_press_event(GdkEventButton* event)
{
    if(event->type == GDK_BUTTON_PRESS)
    {
	if (event->button == 3)
        {
	    Gtk::TreeModel::Path path;
	    Gtk::TreeViewColumn* column;
	    int cell_x;
	    int cell_y; 
	    if (m_TreeView.get_path_at_pos(event->x, event->y, path, column, cell_x, cell_y ))
	    {
		int row_num = *path.get_indices().begin();
		// Gtk::TreeModel::Row row = *( m_refTreeModel->children[row_num - 1]);
		TRACE_DEBUG( << "row_num = " << row_num );
	    }

            Gtk::Menu* popup = getPopupMenuWidget();
            if (popup)
            {
                popup->popup(event->button, event->time);
            }

	    // By returning false also the default handler gets the event:
            return false;
        }
    }
    if(event->type == GDK_2BUTTON_PRESS)
    {
	if (event->button == 1)
        {
	    TRACE_DEBUG( << "Double Click" );
	    Glib::RefPtr<Gtk::TreeView::Selection> selection = m_TreeView.get_selection();
	    Gtk::TreeModel::iterator iter = selection->get_selected();
	    if (iter)
	    {
		Gtk::TreeModel::Row row = *iter;
		TRACE_DEBUG( << "Selected: " << row[m_Columns.m_col_name] );
	    }
	}
    }

    // Event has not been handled:
    return false;
}

void PlayListWindow::on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context,
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
	    int n;
	    Gtk::TreeModel::Path path;
	    if ( m_TreeView.get_path_at_pos(x,y, path) &&
		 path.size() == 1 )
	    {
		n = path[0];
	    }
	    else
	    {
		n = m_PlayList.size();
	    }

	    FileList::iterator it = file_list.begin();
	    while(it != file_list.end())
	    {
		Glib::ustring file = Glib::filename_from_uri(*it);
		TRACE_DEBUG(<< file);
		m_PlayList.insert(n++, file);
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

void PlayListWindow::on_row_changed(const Gtk::TreeModel::Path&  path, const Gtk::TreeModel::iterator&  iter)
{
    // saveConfigurationData();
}

void PlayListWindow::on_row_inserted(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter)
{
    // saveConfigurationData();
}

void PlayListWindow::on_row_deleted(const Gtk::TreeModel::Path& path)
{
    // saveConfigurationData();
}

void PlayListWindow::on_rows_reordered(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter, int* new_order)
{
    // saveConfigurationData();
}

void PlayListWindow::on_play_entry()
{
    TRACE_DEBUG();

    Glib::RefPtr<Gtk::TreeView::Selection> selection = m_TreeView.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (iter)
    {
	playEntry(iter);
    }
}

void PlayListWindow::on_remove_entry()
{
    Glib::RefPtr<Gtk::TreeView::Selection> selection = m_TreeView.get_selection();
    Gtk::TreeModel::iterator it = selection->get_selected();
    if (it)
    {
	if (m_refTreeModel->isCurrent(it))
	{
	    // Use implementation in SignalDispatcher to close the current file:
	    signal_file_close();
	}
	else
	{
	    m_refTreeModel->erase(it);
	}
    }
}

void PlayListWindow::on_clear_all_entries()
{
    m_refTreeModel->clear();

    // Close current file:
    signal_close();
}

void PlayListWindow::playEntry(const Gtk::TreeModel::iterator& it)
{
    m_refTreeModel->setCurrent(it);
    signal_close();
    signal_open();
    signal_play();
}

Gtk::Menu* PlayListWindow::getPopupMenuWidget()
{
    return dynamic_cast<Gtk::Menu*>(m_refUIManager->get_widget("/PopupMenu"));
}

void PlayListWindow::prepareCellRenderer(Gtk::CellRenderer*, Gtk::TreeModel::const_iterator const &row)
{
    bool current = m_refTreeModel->isCurrent(row);

    // see /usr/include/pangomm-1.4/pangomm/fontdescription.h
    m_CellRendererText.property_style() = ( current ?
					    Pango::STYLE_OBLIQUE :
					    Pango::STYLE_NORMAL );
    m_CellRendererText.property_weight() = ( current ?
					     Pango::WEIGHT_ULTRABOLD :
					     Pango::WEIGHT_NORMAL );
}
