//
// Inhibit Screen Saver
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
