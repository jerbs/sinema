//
// Inhibit Screen Saver
//
// Copyright (C) Joachim Erbs
//

#ifndef INHIBIT_SCREEN_SAVER_HPP
#define INHIBIT_SCREEN_SAVER_HPP

#include <boost/shared_ptr.hpp>
#include <gtkmm/widget.h>

class InhibitScreenSaverImpl;

class InhibitScreenSaver
{
public:
    InhibitScreenSaver();

    void on_realize(Gtk::Widget* widget);
    void simulateUserActivity();

private:
    boost::shared_ptr<InhibitScreenSaverImpl> m_impl;
};

#endif
