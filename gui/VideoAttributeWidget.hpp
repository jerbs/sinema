//
// Video Attribute Widget
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

#ifndef VIDEO_ATTRIBUTE_WIDGET_HPP
#define VIDEO_ATTRIBUTE_WIDGET_HPP

#include "player/GeneralEvents.hpp"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>
#include <gtkmm/table.h>

#include <map>

class VideoAttributeWidget : public Gtk::VBox
{
public:
    VideoAttributeWidget();
    ~VideoAttributeWidget();

    void on_notification_video_attribute(const NotificationVideoAttribute&);
    void on_value_changed(Gtk::HScale*, std::string name);

    sigc::signal<void, std::string, int> signalVideoAttributeChanged;

private:
    struct AttributeUI
    {
	AttributeUI(const NotificationVideoAttribute& attr);

	NotificationVideoAttribute attr;
	Gtk::Label m_Label;
	Gtk::HScale m_Scale;
	Gtk::Button m_Button;
    };

    typedef std::map<std::string, AttributeUI*> map_type;
    map_type m_Attributes;

    Gtk::Frame m_VideoAttributeFrame;
    Gtk::Table m_VideoAttributeTable;
};

#endif
