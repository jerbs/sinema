//
// Player Config Widget
//
// Copyright (C) Joachim Erbs, 2010
//

#ifndef PLAYER_CONFIG_WIDGET_HPP
#define PLAYER_CONFIG_WIDGET_HPP

#include "common/GeneralEvents.hpp"
#include "player/GeneralEvents.hpp"

#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/combobox.h>
#include <gtkmm/frame.h>
#include <gtkmm/liststore.h>

#include <list>

class PlayerConfigWidget : public Gtk::VBox
{
public:
    sigc::signal<void, boost::shared_ptr<ConfigurationData> > signalConfigurationDataChanged;
    sigc::signal<void> signalEnableOptimalPixelFormat;
    sigc::signal<void> signalDisableOptimalPixelFormat;
    sigc::signal<void> signalEnableXvClipping;
    sigc::signal<void> signalDisableXvClipping;
    sigc::signal<void, const std::string&> signalSelectDeinterlacer;

    PlayerConfigWidget();
    ~PlayerConfigWidget();

    virtual void on_configuration_data_loaded(boost::shared_ptr<ConfigurationData>);
    void on_deinterlacer_list(const NotificationDeinterlacerList&);

private:
    void on_deinterlacer_changed();
    void on_useOptimalPixelFormat_toggled();
    void on_useXvClipping_toggled();

    void selectConfiguredDeinterlacer();

    Gtk::Frame m_FrameDeinterlacer;
    Gtk::HBox m_HBoxDeinterlacer;
    Gtk::Label m_LabelDeinterlacer; 
    Gtk::ComboBox m_ComboBoxDeinterlacer;

    class ModelColumnsCombo : public Gtk::TreeModel::ColumnRecord
    {
    public:
        ModelColumnsCombo()
        {
            add(m_col_name);
        }

        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
    };
    ModelColumnsCombo m_ColumnsCombo;
    Glib::RefPtr<Gtk::ListStore> m_refTreeModelComboDeinterlacer;

    Gtk::Frame m_FramePixelFormat;
    Gtk::VBox  m_VBoxPixelFormat;
    Gtk::CheckButton m_UseOptimalPixelFormat;
    Gtk::CheckButton m_UseXvClipping;

    boost::shared_ptr<ConfigurationData> m_ConfigurationData;
};

#endif
