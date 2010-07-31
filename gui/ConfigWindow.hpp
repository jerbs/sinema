//
// Config Window
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

#ifndef CONFIG_WINDOW_HPP
#define CONFIG_WINDOW_HPP

#include "receiver/GeneralEvents.hpp"
#include "common/GeneralEvents.hpp"

#include <gtkmm/actiongroup.h>
#include <gtkmm/window.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treeview.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/liststore.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/box.h>
#include <gtkmm/notebook.h>

#include <glibmm/ustring.h>

#include <map>
#include <string>

class ConfigWindow : public Gtk::Window
{
public:
    sigc::signal<void, const ConfigurationData&> signalConfigurationDataChanged;

    ConfigWindow();
    virtual ~ConfigWindow();

    //Signal handlers:
    virtual void on_show_window(bool show);

    void addWidget(Gtk::Widget& widget, const Glib::ustring & tab_label, const Glib::ustring & menu_label);

private:

    // Child widgets:
    Gtk::Notebook m_Notebook;

    bool m_shown;
    int m_pos_x;
    int m_pos_y;
};

#endif
