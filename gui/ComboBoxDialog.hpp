//
// Combo Box Dialog
//
// Copyright (C) Joachim Erbs, 2010
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
