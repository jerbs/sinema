//
// About and Help dialog
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

#ifndef ABOUT_HPP
#define ABOUT_HPP

#include <gtkmm/aboutdialog.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/textview.h>
#include <gtkmm/window.h>

#include "platform/Logging.hpp"

class AboutDialog : public Gtk::AboutDialog
{
public:
    AboutDialog();
    ~AboutDialog();

protected:
    virtual void on_response (int response_id);
};

class HelpDialog : public Gtk::Window
{
public:
    HelpDialog();
    ~HelpDialog();

protected:
    virtual void on_response (int response_id);

private:
    typedef Glib::RefPtr<Gtk::TextBuffer::Tag> TextTagPtr; 

    Gtk::ScrolledWindow m_ScrolledWindow;
    Gtk::TextView m_TextView;
    Glib::RefPtr<Gtk::TextBuffer> m_refTextBuffer;
};

#endif
