//
// Play List Tree Model
//
// Copyright (C) Joachim Erbs, 2010
//

#define NDEBUG
#include <assert.h>

#include "gui/PlayListTreeModel.hpp"
#include "platform/Logging.hpp"

#include <iostream>
#include <sstream>

PlayListTreeModel::PlayListTreeModel(GtkmmPlayList& playList)
: Glib::ObjectBase( typeid(PlayListTreeModel) ), //register a custom GType.
  Glib::Object(), //The custom GType is actually registered here.
  m_PlayList(playList),
  m_columns(1),
  m_stamp(1)  // When the model's stamp != the iterator's stamp 
              // then that iterator is invalid and should be ignored. 
              // Also, 0=invalid
{

    //We need to specify a particular get_type() from one of the virtual base classes, but they should
    //both return the same piece of data.
    Gtk::TreeModel::add_interface( Glib::Object::get_type() );

    playList.signal_entry_changed.connect(sigc::mem_fun(this, &PlayListTreeModel::on_entry_changed));
    playList.signal_entry_inserted.connect(sigc::mem_fun(this, &PlayListTreeModel::on_entry_inserted));
    playList.signal_entry_deleted.connect(sigc::mem_fun(this, &PlayListTreeModel::on_entry_deleted));
}

PlayListTreeModel::~PlayListTreeModel()
{
}

Glib::RefPtr<PlayListTreeModel> PlayListTreeModel::create(GtkmmPlayList& playList)
{
    return Glib::RefPtr<PlayListTreeModel>( new PlayListTreeModel(playList) );
}

// -------------------------------------------------------------------
// TreeModel overrides:

Gtk::TreeModelFlags PlayListTreeModel::get_flags_vfunc() const
{
    // Returns flags supported by this interface.
    // Possible flags are:
    //   TREE_MODEL_ITERS_PERSIST
    //   TREE_MODEL_LIST_ONLY 

    return Gtk::TreeModelFlags(Gtk::TREE_MODEL_LIST_ONLY);
}

int PlayListTreeModel::get_n_columns_vfunc() const
{
    // Returns the number of columns.

    return m_columns;
}

GType PlayListTreeModel::get_column_type_vfunc(int index) const
{
    // Returns the type of the elements in column index.
    assert(index < m_columns);

    return m_TreeModelColumn.type();
}

bool PlayListTreeModel::iter_next_vfunc(const iterator& iter, iterator& iter_next) const
{
    // Sets next_iter to refer to the node following iter at the current level.
    // If there is no next iter, false is returned and iter_next is set to be invalid.

    iter_next = iterator();

    assert(iter_is_valid(iter));

    {
	// Get row iter is pointing to:
	int row_index = get_row_index(iter);
	row_index++;
	if (row_index < m_PlayList.size())
	{
	    // Next row is valid.

	    // Make next_iter valid:
	    iter_next.set_stamp(m_stamp);

	    // Store row in iter_next:
	    set_row_index(iter_next, row_index);

	    // Success
	    return true;
	}
    }

    // Make next_iter invalid:
    iter_next.set_stamp(m_stamp-1);

    // Failed:
    return false;
}

bool PlayListTreeModel::get_iter_vfunc(const Path& path, iterator& iter) const
{
    // Sets iter to the row referenced by path and returns true if possible.
    // If path is invalid, then false is returned and iter is set invalid.

    iter = iterator();

    if(path.size() == 1)
    {
	int row_index = path[0];

	if (row_index < m_PlayList.size())
	{
	    // path is valid.

	    // Make iter valid:
	    iter.set_stamp(m_stamp);

	    // Store row in iter:
	    set_row_index(iter, row_index);
	    return true;
	}
    }

    // Make iter invalid:
    iter.set_stamp(m_stamp-1);
    return false;
}

bool PlayListTreeModel::iter_children_vfunc(const iterator& parent, iterator& iter) const
{
    // Sets iter to refer to the first child of parent. If parent has no children, 
    // false is returned and iter is set to be invalid.

    // parent has no children.

    iter = iterator();

    // Make iter invalid:
    iter.set_stamp(m_stamp-1);

    return false;
}

bool PlayListTreeModel::iter_parent_vfunc(const iterator& child, iterator& iter) const
{
    // Sets iter to be the parent of child. If child is at the toplevel, and doesn't
    // have a parent, then iter is set to an invalid iterator and false is returned.

    // child has no parent.

    iter = iterator();

    // Make iter invalid:
    iter.set_stamp(m_stamp-1);

    return false;
}

bool PlayListTreeModel::iter_nth_child_vfunc(const iterator& parent, int n, iterator& iter) const
{
    // Sets iter to be the child of parent using the given index. The first index is 0. 
    // If n is too big, or parent has no children, iter is set to an invalid iterator 
    // and false is returned.

    // parent has no children.

    iter = iterator();

    // Make iter invalid:
    iter.set_stamp(m_stamp-1);

    return false;
}

bool PlayListTreeModel::iter_nth_root_child_vfunc(int n, iterator& iter) const
{
    // Sets iter to be the child of the root element. The first index is 0. If n is too 
    // big, or if there are no children, iter is set to an invalid iterator and false 
    // is returned.

    iter = iterator();

    assert(n < m_PlayList.size());

    {
	// n is valid.

	// Make iter valid:
	iter.set_stamp(m_stamp);

	// Store row in iter:
	set_row_index(iter, n);

	return true;
    }

    // Make iter invalid:
    iter.set_stamp(m_stamp-1);

    return false;
}

bool PlayListTreeModel::iter_has_child_vfunc(const iterator& iter) const
{
    // Returns true if iter has children, false otherwise.

    // iter has no child.

    return false;
}

int PlayListTreeModel::iter_n_children_vfunc(const iterator& iter) const
{
    // Returns the number of children that iter has.

    // iter has zero children:

    return 0;
}

int PlayListTreeModel::iter_n_root_children_vfunc() const
{
    // Returns the number of toplevel nodes.

    return m_PlayList.size();
}

// ref_node_vfunc: Optional method to improve performance of caching models.
// unref_node_vfunc: Optional method to improve performance of caching models.

Gtk::TreeModel::Path PlayListTreeModel::get_path_vfunc(const iterator& iter) const
{
    // Returns a Path referenced by iter.

    int row_index = get_row_index(iter);
    Path path;
    path.push_back(row_index);
    return path;
}

void PlayListTreeModel::get_value_vfunc(const TreeModel::iterator& iter, int column, Glib::ValueBase& value) const
{
    // Initializes and sets value to that at column in the row referenced by iter.

    assert(iter_is_valid(iter));

    {
	if(column < m_columns)
	{
	    int row_index = get_row_index(iter);
	    std::string str = m_PlayList[row_index];

	    if (row_index == m_PlayList.getCurrentIndex())
	    {
		str = "* " + str;
	    }

	    // value and val contains data and type info.

	    // Here type_TreeModelColumn::ValueType is Glib::Value<Glib::ustring>.
	    type_TreeModelColumn::ValueType val;
	    val.init(type_TreeModelColumn::ValueType::value_type());
	    val.set(str);

	    // Copy:
	    value.init(Glib::Value< Glib::ustring >::value_type());
	    value = val;
	}
    }
}

#if 0
bool PlayListTreeModel::iter_is_valid(const iterator& iter) const
{
    return iter && m_stamp == iter.get_stamp() && Gtk::TreeModel::iter_is_valid(iter);
}
#endif

// -------------------------------------------------------------------
// TreeDragSource overrides:

bool PlayListTreeModel::row_draggable_vfunc(const TreeModel::Path&  path) const
{
    // Returns whether a particular row can be used as the source of a DND operation. 

    DEBUG();

    // All rows are draggable:
    return true;
}

bool PlayListTreeModel::drag_data_get_vfunc(const TreeModel::Path& path, Gtk::SelectionData& selection_data) const
{
    // Fills in selection_data with a representation of the row at path.
    // selection_data->target gives the required type of the data.
    // Should robustly handle a path no longer found in the model!
    // Returns true if the required data was provided.

    DEBUG();

    if(path.size() == 1)
    {
	int row_index = path[0];
	if (row_index < m_PlayList.size())
	{
	    // path is valid.
	    std::string type = "<file>";
	    if (row_index == m_PlayList.getCurrentIndex())
	    {
		type += "<current>";
	    }
	    std::string data = m_PlayList[row_index];
	    selection_data.set(type, data);

	    return true;
	}
    }

    return false;
}

bool PlayListTreeModel::drag_data_delete_vfunc(const TreeModel::Path& path)
{
    // Deletes the row at path, because it was moved somewhere else via drag-and-drop.
    // Returns false if the deletion fails because path no longer exists, or for some model-specific reason.
    // Should robustly handle a path no longer found in the model!
    // Returns true if the row was successfully deleted.

    DEBUG();

    if(path.size() == 1)
    {
	int row_index = path[0];
	if (row_index < m_PlayList.size())
	{
	    return m_PlayList.erase(row_index);
	}
    }

    return false;
}

// -------------------------------------------------------------------
// TreeDragDest overrides:

bool PlayListTreeModel::drag_data_received_vfunc(const TreeModel::Path&  dest, const Gtk::SelectionData&  selection_data)
{
    // Insert a row before the path dest, deriving the contents of the row from selection_data.
    // If dest is outside the tree so that inserting before it is impossible, false will be returned.
    // Also, false may be returned if the new row is not created for some model-specific reason.
    // Should robustly handle a dest no longer found in the model! 

    DEBUG();

    if (selection_data.get_data_type().find("<file>") != std::string::npos &&
	selection_data.get_target() == "GTK_TREE_MODEL_ROW")
    {
	if(dest.size() == 1)
	{
	    int row_index = dest[0];
	    DEBUG(<< "row_index = " << row_index << ", " << selection_data.get_data_as_string());
	    if (row_index <= m_PlayList.size())
	    {
		PlayList::iterator it = m_PlayList.insert(row_index, selection_data.get_data_as_string());
		if (selection_data.get_data_type().find("<current>") != std::string::npos)
		{
		    m_PlayList.select(it);
		}
		return (it != m_PlayList.end());
	    }
	}
    }

    return false;
}

bool PlayListTreeModel::row_drop_possible_vfunc(const TreeModel::Path& dest, const Gtk::SelectionData& selection_data) const
{
    // Determines whether a drop is possible before the given dest_path, at the same depth as dest_path.
    // i.e., can we drop the data in selection_data at that location. dest_path does not have to exist;
    // the return value will almost certainly be false if the parent of dest_path doesn't exist, though.

    DEBUG(<< "Type  : " << selection_data.get_data_type());
    DEBUG(<< "String: " << selection_data.get_data_as_string());
    DEBUG(<< "Target: " << selection_data.get_target());

    if (selection_data.get_data_type().find("<file>") != std::string::npos &&
	selection_data.get_target() == "GTK_TREE_MODEL_ROW")
    {
	if(dest.size() == 1)
	{
	    int row_index = dest[0];
	    DEBUG(<< "row_index = " << row_index);

	    if (row_index < m_PlayList.size())
	    {
		// dest is valid.
		return true;
	    }
	}
    }

    return false;
}

// -------------------------------------------------------------------

void PlayListTreeModel::on_entry_changed(const GtkmmPlayList::path& p, const PlayList::iterator&)
{
    DEBUG();
    if (p.size())
    {
	unsigned int n = p[0];
	TreeModel::Path path;
	path.push_back(n); 
	TreeModel::iterator it = get_iter(path);
	row_changed(path, it);
    }
}

void PlayListTreeModel::on_entry_inserted(const GtkmmPlayList::path& p, const PlayList::iterator&)
{
    m_stamp++;
    if (p.size())
    {
	unsigned int n = p[0];
	TreeModel::Path path;
	path.push_back(n); 
	TreeModel::iterator it = get_iter(path);
	row_inserted(path, it);
    }
}

void PlayListTreeModel::on_entry_deleted(const GtkmmPlayList::path& p)
{
    m_stamp++;
    if (p.size())
    {
	int n = p[0];
	TreeModel::Path path;
	path.push_back(n); 
	row_deleted(path);
    }
}

// -------------------------------------------------------------------
// The row referenced by an iterator is directly stored in the user_data pointer.

int PlayListTreeModel::get_row_index(const TreeModel::iterator& iter) const
{
    const void* addr = &(iter.gobj()->user_data);
    int* iaddr = (int*)addr;
    int row_index = *iaddr;
    return row_index;
}

void PlayListTreeModel::set_row_index(const TreeModel::iterator& iter, int row_index) const
{
    const void* addr = &(iter.gobj()->user_data);
    int* iaddr = (int*)addr;
    *iaddr = row_index;
}
