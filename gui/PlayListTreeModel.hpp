//
// Play List Tree Model
//
// Copyright (C) Joachim Erbs, 2010
//

#ifndef PLAY_LIST_TREE_MODEL_HPP
#define PLAY_LIST_TREE_MODEL_HPP

#include "gui/GtkmmPlayList.hpp"

#include <gtkmm/treemodel.h>
#include <gtkmm/treepath.h>

class PlayListTreeModel
  : public Glib::Object,
    public Gtk::TreeModel
{
public:
    PlayListTreeModel(GtkmmPlayList& playList);
    virtual ~PlayListTreeModel();

    static Glib::RefPtr<PlayListTreeModel> create(GtkmmPlayList& playList);

protected:
    virtual Gtk::TreeModelFlags get_flags_vfunc() const;
    virtual int get_n_columns_vfunc() const;
    virtual GType get_column_type_vfunc(int index) const;
    virtual bool iter_next_vfunc(const iterator& iter, iterator& iter_next) const;
    virtual bool get_iter_vfunc(const Path& path, iterator& iter) const;
    virtual bool iter_children_vfunc(const iterator& parent, iterator& iter) const;
    virtual bool iter_parent_vfunc(const iterator& child, iterator& iter) const;
    virtual bool iter_nth_child_vfunc(const iterator& parent, int n, iterator& iter) const;
    virtual bool iter_nth_root_child_vfunc(int n, iterator& iter) const;
    virtual bool iter_has_child_vfunc(const iterator& iter) const;
    virtual int iter_n_children_vfunc(const iterator& iter) const;
    virtual int iter_n_root_children_vfunc() const;
    // ref_node_vfunc
    // unref_node_vfunc
   virtual Path get_path_vfunc(const iterator& iter) const;
   virtual void get_value_vfunc(const TreeModel::iterator& iter, int column, Glib::ValueBase& value) const;
    // for debugging only virtual bool iter_is_valid(const iterator& iter) const;
    // set_value_impl
    // get_value_impl
    // on_row_changed
    // on_row_inserted
    // on_row_has_child_toggled
    // on_row_deleted
    // on_rows_reordered

private:
    void on_entry_changed(const GtkmmPlayList::path&, const PlayList::iterator&);
    void on_entry_inserted(const GtkmmPlayList::path&, const PlayList::iterator&);
    void on_entry_deleted(const GtkmmPlayList::path&);
    void on_active_entry_changed(const GtkmmPlayList::path&, const PlayList::iterator&);

    GtkmmPlayList& m_PlayList;

    int m_columns;

    int get_row_index(const TreeModel::iterator& iter) const;
    void set_row_index(const TreeModel::iterator& iter, int row) const;

    //  bool is_valid(const TreeModel::iterator& iter) const;

    typedef Gtk::TreeModelColumn<Glib::ustring> type_TreeModelColumn;
    type_TreeModelColumn m_TreeModelColumn;

    int m_stamp; //When the model's stamp and the TreeIter's stamp are equal, the TreeIter is valid.
};

#endif
