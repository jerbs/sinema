//
// Combo Box Dialog
//
// Copyright (C) Joachim Erbs, 2010
//
//    This file is part of Sinema.
//
//    Sinema is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Sinema is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Sinema.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef COMBO_BOX_DIALOG_HPP
#define COMBO_BOX_DIALOG_HPP

#include "gtkmm/dialog.h"
#include "gtkmm/combobox.h"
#include "gtkmm/frame.h"
#include "gtkmm/stock.h"
#include "glibmm/ustring.h"

template<class T>
class ComboBoxDialog : public Gtk::Dialog
{
public:
    typedef T ElementType;

    ComboBoxDialog(const Glib::RefPtr<Gtk::TreeModel>& model,
		   const Gtk::TreeModelColumn<T>& column,
		   const Glib::ustring& action_name,
		   const Glib::ustring& frame_name)
	: m_Frame(frame_name),
	  m_ComboBox(model),
	  m_column(column)
    {
	m_ComboBox.pack_start(column);
	m_Frame.add(m_ComboBox);
	get_vbox()->pack_start(m_Frame, Gtk::PACK_SHRINK);

	add_button(Gtk::Stock::CANCEL, 0);
	add_button(action_name, 1);
	show_all_children();
    }

    T get_selected_value()
    {
	Gtk::TreeModel::iterator iter = m_ComboBox.get_active();
	if(iter)
	{
	    Gtk::TreeModel::Row row = *iter;
	    return row[m_column];
	}
	else
	{
	    return T();
	}
    }

private:
    Gtk::Frame m_Frame;
    Gtk::Label m_Label;
    Gtk::ComboBox m_ComboBox;
    Gtk::TreeModelColumn<T> m_column;
};

#endif
