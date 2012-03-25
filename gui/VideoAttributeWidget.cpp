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

#include "gui/VideoAttributeWidget.hpp"
#include "platform/temp_value.hpp"
#include <algorithm>

VideoAttributeWidget::VideoAttributeWidget()
    : m_VideoAttributeFrame("Attribute"),
      m_VideoAttributeTable(1,3),  // Using 0 rows results in a runtime warning.
      m_emitChangedSignalFlag(true)
{
    set_spacing(4);
    set_border_width(4);

    pack_start(m_VideoAttributeFrame, Gtk::PACK_SHRINK);
    m_VideoAttributeFrame.add(m_VideoAttributeTable);

    m_VideoAttributeTable.set_col_spacing(0, 20);
    m_VideoAttributeTable.set_col_spacing(1, 20);
}

VideoAttributeWidget::~VideoAttributeWidget()
{
}

void VideoAttributeWidget::on_notification_video_attribute(const NotificationVideoAttribute& attr)
{
    map_type::iterator it = m_Attributes.find(attr.name);
    if (it == m_Attributes.end())
    {
	if (attr.valid_config)
	{
	    // Add new attribute:
	    AttributeUI* aui = new AttributeUI(attr);
	    m_Attributes.insert(std::pair<std::string, AttributeUI*>(attr.name, aui));

	    guint rows;
	    guint cols;
	    m_VideoAttributeTable.get_size(rows, cols);
	    m_VideoAttributeTable.resize(rows+1, cols);

	    m_VideoAttributeTable.attach(aui->m_Label, 0, 1, rows, rows+1, Gtk::SHRINK, Gtk::SHRINK, 0, 0);
	    m_VideoAttributeTable.attach(aui->m_Scale, 1, 2, rows, rows+1, Gtk::FILL | Gtk::EXPAND, Gtk::FILL | Gtk::EXPAND, 0, 0);
	    m_VideoAttributeTable.attach(aui->m_Button, 2, 3, rows, rows+1, Gtk::SHRINK, Gtk::SHRINK, 0, 0);

	    aui->m_Scale.signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &VideoAttributeWidget::on_value_changed),
								   &(aui->m_Scale), attr.name));
	    // The button resets to the initial value:
	    aui->m_Button.signal_clicked().connect(sigc::bind(sigc::mem_fun(aui->m_Scale, &Gtk::HScale::set_value), attr.value));
	}
    }
    else
    {
	// Update previously configured AttributeUI:
	AttributeUI* aui = it->second;
	TemporaryDisable disable(m_emitChangedSignalFlag);
	aui->m_Scale.set_value(attr.value);
    }

    show_all();
}

VideoAttributeWidget::AttributeUI::AttributeUI(const NotificationVideoAttribute& attr)
    : attr(attr),
      m_Label(attr.name),
      m_Scale(attr.min_value, attr.max_value, 0),
      m_Button("Reset")
{
    m_Scale.set_value(attr.value);

    // Display current value as string next to the slider:
    m_Scale.set_draw_value(true);
    m_Scale.set_value_pos(Gtk::POS_TOP);

    Glib::ustring none;
    unsigned int step = (attr.max_value - attr.min_value) / 4;
    if (step == 0) step = 1;
    for (int i = attr.min_value; i <= attr.max_value; i += step)
    {
	m_Scale.add_mark(i, Gtk::POS_BOTTOM, none);
    }

    double step_size = 1;
    double page_size = std::max(1, (attr.max_value - attr.min_value) / 20);
    m_Scale.set_increments(step_size, page_size);

    // Fill level is used to indicate the initial value:
    m_Scale.set_fill_level(attr.value);
    m_Scale.set_show_fill_level(true);
    m_Scale.set_restrict_to_fill_level(false);
}

void VideoAttributeWidget::on_value_changed(Gtk::HScale* scale, std::string name)
{
    int value = scale->get_value();
    if (m_emitChangedSignalFlag)
    {
	signalVideoAttributeChanged(name, value);
    }
}
