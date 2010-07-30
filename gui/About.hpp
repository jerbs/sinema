//
// About and Help dialog
//
// Copyright (C) Joachim Erbs, 2010
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
