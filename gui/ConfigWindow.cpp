//
// Config Window
//
// Copyright (C) Joachim Erbs, 2010
//

#include "gui/ConfigWindow.hpp"
#include "gui/ComboBoxDialog.hpp"
#include "receiver/ChannelFrequencyTable.hpp"
#include "platform/Logging.hpp"
#include "platform/temp_value.hpp"

#include <gtkmm/cellrenderercombo.h>
#include <gtkmm/cellrendererspin.h>

#include <stdlib.h>

extern std::string applicationName;

// #undef DEBUG 
// #define DEBUG(text) std::cout << __PRETTY_FUNCTION__ text << std::endl;

ConfigWindow::ConfigWindow()
    : m_shown(false),
      m_pos_x(0),
      m_pos_y(0)
{
    set_title(applicationName + " (Configuration)");
    set_default_size(400,200);

    add(m_Notebook);

    show_all_children();
}

ConfigWindow::~ConfigWindow()
{
}

void ConfigWindow::on_show_window(bool pshow)
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

void ConfigWindow::addWidget(Gtk::Widget& widget, const Glib::ustring & tab_label, const Glib::ustring & menu_label)
{
    m_Notebook.append_page(widget, tab_label, menu_label);
    show_all_children();
}
