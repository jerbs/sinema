//
// Inhibit Screen Saver
//
// Copyright (C) Joachim Erbs, 2010
//

#ifndef INHIBIT_SCREEN_SAVER_HPP
#define INHIBIT_SCREEN_SAVER_HPP

#include <boost/shared_ptr.hpp>
#include <gtkmm/widget.h>

class XScreenSaverInterface;

class InhibitScreenSaver
{
public:
    InhibitScreenSaver();

    void on_realize(Gtk::Widget* widget);
    void simulateUserActivity();

private:
    boost::shared_ptr<XScreenSaverInterface> m_XScreenSaverInterface;
};

#endif
