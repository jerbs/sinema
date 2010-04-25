//
// Play List Window
//
// Copyright (C) Joachim Erbs, 2010
//

#include "gui/PlayListWindow.hpp"
#include "platform/Logging.hpp"

extern std::string applicationName;

PlayListWindow::PlayListWindow(GtkmmPlayList& playList)
    : m_shown(false),
      m_pos_x(0),
      m_pos_y(0)
{
    set_title(applicationName + " (Play List)");
    set_default_size(400,200);

    m_ScrolledWindow.add(m_TreeView);
    m_Box.pack_start(m_ScrolledWindow, Gtk::PACK_EXPAND_WIDGET);
    m_Box.pack_end(m_StatusBar, Gtk::PACK_SHRINK);
    m_StatusBar.set_spacing(15);
    m_StatusBar.pack_start(m_StatusBarMessage, Gtk::PACK_EXPAND_WIDGET);
    add(m_Box);

    // Show scrollbars only when necessary:
    m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    //Create the Tree model:
    // m_refTreeModel = PlayListTreeModel::create(m_Columns);
    m_refTreeModel = PlayListTreeModel::create(playList);
    m_TreeView.set_model(m_refTreeModel);

    //Add the TreeView's view columns:
    m_TreeView.append_column("Name", m_Columns.m_col_name);

    // Allow rows to be drag and dropped within the treeview:
    m_TreeView.set_reorderable();

    // Context Menu
    // Create actions for menus and toolbars:
    m_refActionGroup = Gtk::ActionGroup::create();
    m_refActionGroup->add(Gtk::Action::create("PlayEntry", "Play Entry"),
                          sigc::mem_fun(*this, &PlayListWindow::on_play_entry));
    m_refActionGroup->add(Gtk::Action::create("AddEntryBefore", "Add Entry Before"),
                          sigc::mem_fun(*this, &PlayListWindow::on_add_entry_before));
    m_refActionGroup->add(Gtk::Action::create("AddEntryAfter", "Add Entry After"),
                          sigc::mem_fun(*this, &PlayListWindow::on_add_entry_after));
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
        "    <menuitem action='AddEntryBefore'/>"
        "    <menuitem action='AddEntryAfter'/>"
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
    DEBUG();

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
    DEBUG();

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
		DEBUG( << "row_num = " << row_num );
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
	    DEBUG( << "Double Click" );
	    Glib::RefPtr<Gtk::TreeView::Selection> selection = m_TreeView.get_selection();
	    Gtk::TreeModel::iterator iter = selection->get_selected();
	    if (iter)
	    {
		Gtk::TreeModel::Row row = *iter;
		DEBUG( << "Selected: " << row[m_Columns.m_col_name] );
	    }
	}
    }

    // Event has not been handled:
    return false;
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
    DEBUG();

    Glib::RefPtr<Gtk::TreeView::Selection> selection = m_TreeView.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (iter)
    {
	playEntry(iter);
    }
}

void PlayListWindow::on_add_entry_before()
{
}

void PlayListWindow::on_add_entry_after()
{
}

void PlayListWindow::on_remove_entry()
{
}

void PlayListWindow::on_clear_all_entries()
{
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
