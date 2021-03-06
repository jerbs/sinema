//
// Channel Config Widget
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

#ifndef CHANNEL_CONFIG_WIDGET_HPP
#define CHANNEL_CONFIG_WIDGET_HPP

#include "receiver/GeneralEvents.hpp"
#include "common/GeneralEvents.hpp"

#include <gtkmm/actiongroup.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treeview.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/liststore.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/box.h>

#include <glibmm/ustring.h>

#include <map>
#include <string>

class ChannelConfigWidget : public Gtk::VBox
{
public:
    sigc::signal<void, const ChannelData&> signalSetFrequency;
    sigc::signal<void, std::string> signalStartScan;
    sigc::signal<void, boost::shared_ptr<ConfigurationData> > signalConfigurationDataChanged;

    ChannelConfigWidget();
    virtual ~ChannelConfigWidget();

    //Signal handlers:

    virtual void on_tuner_channel_tuned(const ChannelData& channelData);
    virtual void on_tuner_signal_detected(const ChannelData& channelData);
    virtual void on_tuner_scan_stopped();
    virtual void on_tuner_scan_finished();

    virtual void on_configuration_data_loaded(boost::shared_ptr<ConfigurationData>);

private:
    //Signal handlers:
    virtual void on_cellrenderer_standard_edited(const Glib::ustring& path_string,
						 const Glib::ustring& new_text);
    virtual void on_cellrenderer_channel_edited(const Glib::ustring& path_string,
						 const Glib::ustring& new_text);
    virtual void on_cellrenderer_finetune_edited(const Glib::ustring& path_string,
						 const Glib::ustring& new_text);
    virtual void on_row_activated (const Gtk::TreeModel::Path& path,
				   Gtk::TreeViewColumn* column);
    virtual bool on_button_press_event(GdkEventButton* event);

    virtual void on_row_changed(const Gtk::TreeModel::Path&  path, const Gtk::TreeModel::iterator&  iter);
    virtual void on_row_inserted(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter);
    virtual void on_row_deleted(const Gtk::TreeModel::Path& path);
    virtual void on_rows_reordered(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter, int* new_order);

    virtual void on_tune_channel();
    virtual void on_add_entry_before();
    virtual void on_add_entry_after();
    virtual void on_remove_entry();
    virtual void on_scan_channels();

    void saveConfigurationData();

    void setStandard(Gtk::TreeRow& row, const Glib::ustring& standard);
    void setChannel(Gtk::TreeRow& row, const Glib::ustring& channel);
    void updateFrequency(Gtk::TreeRow& row);

    void tuneChannel(const Gtk::TreeRow& row);

    Gtk::Menu* getPopupMenuWidget();

    //Tree model columns:
    class ModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:

	ModelColumns()
	{
	    add(m_col_name);
	    add(m_col_standard);
	    add(m_col_channel);
	    add(m_col_channel_choices);
	    add(m_col_frequency);
	    add(m_col_finetune);
	    add(m_col_signal);
	}

	Gtk::TreeModelColumn<Glib::ustring> m_col_name;
	Gtk::TreeModelColumn<Glib::ustring> m_col_standard;
	Gtk::TreeModelColumn<Glib::ustring> m_col_channel;
	Gtk::TreeModelColumn<Glib::RefPtr<Gtk::TreeModel> > m_col_channel_choices;
	Gtk::TreeModelColumn<unsigned int> m_col_frequency;
	Gtk::TreeModelColumn<int> m_col_finetune;
	Gtk::TreeModelColumn<bool> m_col_signal;
    };

    ModelColumns m_Columns;

    //Tree model columns for the Combo CellRenderer in the TreeView column:
    class ModelColumnsCombo : public Gtk::TreeModel::ColumnRecord
    {
    public:
	ModelColumnsCombo()
	{
	    add(m_col_choice);
	}

	Gtk::TreeModelColumn<Glib::ustring> m_col_choice;
    };
  
    ModelColumnsCombo m_ColumnsCombo;

    // Child widgets:
    Gtk::ScrolledWindow m_ScrolledWindow;
    Gtk::TreeView m_TreeView;
    Gtk::Statusbar m_StatusBar;
    Gtk::Label m_StatusBarMessage;

    Glib::RefPtr<Gtk::ListStore> m_refTreeModel;
    Glib::RefPtr<Gtk::ListStore> m_refTreeModelComboStandard;

    typedef std::map<std::string, Glib::RefPtr<Gtk::ListStore> > ChannelListMap_t;
    ChannelListMap_t m_ChannelListMap;

    Gtk::Adjustment m_FinetuneAdjustment;

    // Popup menu:
    Glib::RefPtr<Gtk::UIManager> m_refUIManager;
    Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup;

    // saveConfigurationData() is enabled when this flag is true. The flag 
    // is used to avoid saving the configuration file unnecessarily. Writing 
    // to the tree modul, i.e. row[m_Columns....] = ..., generates the same 
    // signal as when the user edits the table using the GUI. In the second
    // case configuration data has to be saved. In the first case this is 
    // not useful, when further modification do follow.
    bool m_isEnabled_signalConfigurationDataChanged;

    boost::shared_ptr<ConfigurationData> m_ConfigurationData;
};

#endif
