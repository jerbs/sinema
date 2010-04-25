//
// Play List Window
//
// Copyright (C) Joachim Erbs, 2010
//

#ifndef PLAY_LIST_WINDOW_HPP
#define PLAY_LIST_WINDOW_HPP

#include "gui/GtkmmPlayList.hpp"
#include "gui/PlayListTreeModel.hpp"

#include <gtkmm/actiongroup.h>
#include <gtkmm/window.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treeview.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/liststore.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/box.h>

#include <glibmm/ustring.h>

#include <string>

class PlayListWindow : public Gtk::Window
{
public:
    sigc::signal<void> signal_open;
    sigc::signal<void> signal_play;
    sigc::signal<void> signal_close;

    PlayListWindow(GtkmmPlayList& playList);
    virtual ~PlayListWindow();

    //Signal handlers:
    virtual void on_show_window(bool show);

private:
    //Signal handlers:
    virtual void on_row_activated (const Gtk::TreeModel::Path& path,
				   Gtk::TreeViewColumn* column);
    virtual bool on_button_press_event(GdkEventButton* event);

    virtual void on_row_changed(const Gtk::TreeModel::Path&  path, const Gtk::TreeModel::iterator&  iter);
    virtual void on_row_inserted(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter);
    virtual void on_row_deleted(const Gtk::TreeModel::Path& path);
    virtual void on_rows_reordered(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter, int* new_order);

    virtual void on_play_entry();
    virtual void on_add_entry_before();
    virtual void on_add_entry_after();
    virtual void on_remove_entry();
    virtual void on_clear_all_entries();

    void playEntry(const Gtk::TreeModel::iterator&);

    Gtk::Menu* getPopupMenuWidget();

    //Tree model columns:
    class ModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:

	ModelColumns()
	{
	    add(m_col_name);
	}

	Gtk::TreeModelColumn<Glib::ustring> m_col_name;
    };

    ModelColumns m_Columns;

    // Child widgets:
    Gtk::VBox m_Box;
    Gtk::ScrolledWindow m_ScrolledWindow;
    Gtk::TreeView m_TreeView;
    Gtk::Statusbar m_StatusBar;
    Gtk::Label m_StatusBarMessage;

    Glib::RefPtr<PlayListTreeModel> m_refTreeModel;

    // Popup menu:
    Glib::RefPtr<Gtk::UIManager> m_refUIManager;
    Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup;

    bool m_shown;
    int m_pos_x;
    int m_pos_y;
};

#endif
