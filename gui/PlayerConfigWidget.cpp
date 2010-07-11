//
// Player Config Widget
//
// Copyright (C) Joachim Erbs, 2010
//

#include "gui/PlayerConfigWidget.hpp"

// #undef DEBUG 
// #define DEBUG(text) std::cout << __PRETTY_FUNCTION__ text << std::endl;

PlayerConfigWidget::PlayerConfigWidget()
    : m_FrameDeinterlacer("Deinterlacer"),
      m_LabelDeinterlacer("Deinterlacer"),
      m_FramePixelFormat("Pixel Format"),
      m_UseOptimalPixelFormat("Use optimal pixel format"),
      m_UseXvClipping("Use Xv clipping")
{
    set_spacing(4);
    set_border_width(4);

    pack_start(m_FrameDeinterlacer, Gtk::PACK_SHRINK);
    m_FrameDeinterlacer.add(m_HBoxDeinterlacer);
    m_HBoxDeinterlacer.set_spacing(10);
    m_HBoxDeinterlacer.set_border_width(5);
    m_HBoxDeinterlacer.pack_start(m_LabelDeinterlacer, Gtk::PACK_SHRINK);
    m_HBoxDeinterlacer.pack_start(m_ComboBoxDeinterlacer, Gtk::PACK_EXPAND_WIDGET);
    m_refTreeModelComboDeinterlacer = Gtk::ListStore::create(m_ColumnsCombo);
    m_ComboBoxDeinterlacer.set_model(m_refTreeModelComboDeinterlacer);
    m_ComboBoxDeinterlacer.pack_start(m_ColumnsCombo.m_col_name);

    pack_start(m_FramePixelFormat, Gtk::PACK_SHRINK);
    m_FramePixelFormat.add(m_VBoxPixelFormat);
    m_VBoxPixelFormat.set_spacing(0);
    m_VBoxPixelFormat.set_border_width(5);
    m_VBoxPixelFormat.pack_start(m_UseOptimalPixelFormat, Gtk::PACK_SHRINK);
    m_VBoxPixelFormat.pack_start(m_UseXvClipping, Gtk::PACK_SHRINK);

    // Signals:
    m_ComboBoxDeinterlacer.signal_changed().connect(sigc::mem_fun(this, &PlayerConfigWidget::on_deinterlacer_changed) );
    m_UseOptimalPixelFormat.signal_toggled().connect(sigc::mem_fun(this, &PlayerConfigWidget::on_useOptimalPixelFormat_toggled) );
    m_UseXvClipping.signal_toggled().connect(sigc::mem_fun(this, &PlayerConfigWidget::on_useXvClipping_toggled) );
}

PlayerConfigWidget::~PlayerConfigWidget()
{
}

void PlayerConfigWidget::on_configuration_data_loaded(boost::shared_ptr<ConfigurationData> event)
{
    DEBUG();

    m_ConfigurationData = event;

    m_UseOptimalPixelFormat.set_active(m_ConfigurationData->configPlayer.useOptimalPixelFormat);
    m_UseXvClipping.set_active(m_ConfigurationData->configPlayer.useXvClipping);

    // Maybe NotificationDeinterlacerList is already received:
    selectConfiguredDeinterlacer();

    // Update MediaPlayer with configuration:
    on_deinterlacer_changed();
    on_useOptimalPixelFormat_toggled();
    on_useXvClipping_toggled();
}

void PlayerConfigWidget::on_deinterlacer_list(const NotificationDeinterlacerList& event)
{
    DEBUG();

    m_refTreeModelComboDeinterlacer->clear();    

    typedef std::list<std::string> list_t;
    const list_t& list = event.list;

    for(list_t::const_iterator it = list.begin(); it != list.end(); it++)
    {
	DEBUG(<< *it);
	Gtk::TreeModel::Row row = *(m_refTreeModelComboDeinterlacer->append());
	row[m_ColumnsCombo.m_col_name] = Glib::ustring(*it);
    }

    if (event.selected != -1)
	m_ComboBoxDeinterlacer.set_active(event.selected);

    // Maybe ConfigurationData is already received:
    if (m_ConfigurationData)
    {
	selectConfiguredDeinterlacer();
    }
}

void PlayerConfigWidget::on_deinterlacer_changed()
{
    DEBUG();

    Gtk::TreeModel::iterator iter = m_ComboBoxDeinterlacer.get_active();
    if (iter)
    {
	Gtk::TreeRow row = *iter;
	Glib::ustring n = row[m_ColumnsCombo.m_col_name];
	const std::string name = n;
	signalSelectDeinterlacer(name);

	if (m_ConfigurationData)
	{
	    if (m_ConfigurationData->configPlayer.deinterlacer != name)
	    {
		m_ConfigurationData->configPlayer.deinterlacer = name;
		signalConfigurationDataChanged(m_ConfigurationData);
	    }
	}
    }
}

void PlayerConfigWidget::on_useOptimalPixelFormat_toggled()
{
    DEBUG();

    bool active = m_UseOptimalPixelFormat.get_active();

    if (active)
    {
	signalEnableOptimalPixelFormat();
    }
    else
    {
	signalDisableOptimalPixelFormat();
    }

    if (m_ConfigurationData)
    {
	if (m_ConfigurationData->configPlayer.useOptimalPixelFormat != active)
	{
	    m_ConfigurationData->configPlayer.useOptimalPixelFormat = active;
	    signalConfigurationDataChanged(m_ConfigurationData);
	}
    }
}

void PlayerConfigWidget::on_useXvClipping_toggled()
{
    DEBUG();

    bool active = m_UseXvClipping.get_active();

    if (active)
    {
	signalEnableXvClipping();
    }
    else
    {
	signalDisableXvClipping();
    }

    if (m_ConfigurationData)
    {
	if (m_ConfigurationData->configPlayer.useXvClipping != active)
	{
	    m_ConfigurationData->configPlayer.useXvClipping = active;
	    signalConfigurationDataChanged(m_ConfigurationData);
	}
    }
}

void PlayerConfigWidget::selectConfiguredDeinterlacer()
{
    Gtk::TreePath path;
    path.push_back(0);
    Gtk::ListStore::const_iterator it = m_refTreeModelComboDeinterlacer->get_iter(path);
    while (it)
    {
	Gtk::TreeModel::Row row = *it;

	if (row[m_ColumnsCombo.m_col_name] == m_ConfigurationData->configPlayer.deinterlacer)
	{
	    DEBUG( << "-->" <<row[m_ColumnsCombo.m_col_name]);
	    m_ComboBoxDeinterlacer.set_active(it);
	    return;
	}

	it++;
    }
}
