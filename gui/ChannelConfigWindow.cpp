//
// Channel Config Window
//
// Copyright (C) Joachim Erbs, 2010
//

#include "gui/ChannelConfigWindow.hpp"
#include "gui/ComboBoxDialog.hpp"
#include "receiver/ChannelFrequencyTable.hpp"
#include "platform/Logging.hpp"
#include "platform/temp_value.hpp"

#include <gtkmm/cellrenderercombo.h>
#include <gtkmm/cellrendererspin.h>

#include <stdlib.h>

extern std::string applicationName;

// #undef DEBUG 
// #define DEBUG(text) std::cout << __PRETTY_FUNCTION__ text << std::endl;

ChannelConfigWindow::ChannelConfigWindow()
    : m_FinetuneAdjustment(0, -100, 100),
      dont_save(false)
{
    set_title(applicationName + " (Channel Configuration)");
    set_default_size(400,200);

    m_ScrolledWindow.add(m_TreeView);
    m_Box.pack_start(m_ScrolledWindow, Gtk::PACK_EXPAND_WIDGET);
    m_Box.pack_end(m_StatusBar, Gtk::PACK_SHRINK);
    m_StatusBar.set_spacing(15);
    m_StatusBar.pack_start(m_StatusBarMessage, Gtk::PACK_EXPAND_WIDGET);
    add(m_Box);

    // Show scrollbars only when necessary:
    m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    //Create the Tree model:
    m_refTreeModel = Gtk::ListStore::create(m_Columns);
    m_TreeView.set_model(m_refTreeModel);

    //Create and fill the combo models:
    m_refTreeModelComboStandard = Gtk::ListStore::create(m_ColumnsCombo);
    int i = 0;
    while(const char* standard = ChannelFrequencyTable::getStandard_cstr(i))
    {
	Gtk::TreeModel::Row row = *(m_refTreeModelComboStandard->append());
	row[m_ColumnsCombo.m_col_choice] = standard;

	Glib::RefPtr<Gtk::ListStore> refTreeModel = Gtk::ListStore::create(m_ColumnsCombo);
	ChannelFrequencyTable cft = ChannelFrequencyTable::create(standard);
	int ch = 0;
	while(const char* channelName = ChannelFrequencyTable::getChannelName_cstr(cft, ch))
	{
	    Gtk::TreeModel::Row row = *(refTreeModel->append());
	    row[m_ColumnsCombo.m_col_choice] = channelName;
	    ch++;
	}

	m_ChannelListMap[std::string(standard)] = refTreeModel;

	i++;
    }

    // --- Id column ---
    // Gtk::TreeView::Column* pColumnId = Gtk::manage( new Gtk::TreeView::Column("Id") );
    
    
    // --- Standard column ---

    // CellRendererCombo for column Standard:
    Gtk::CellRendererCombo* pRendererStandard = Gtk::manage( new Gtk::CellRendererCombo);
    // Use the same combo model in all rows:
    pRendererStandard->property_model() = m_refTreeModelComboStandard;
    // Use text from column 0 (m_col_choice) of m_refTreeModelComboStandard:
    pRendererStandard->property_text_column() = 0; 
    // Allow the user to edit the column:
    pRendererStandard->property_editable() = true;
    // Only allow to use values defined in combo box:
    pRendererStandard->property_has_entry() = false;
    pRendererStandard->signal_edited().connect( sigc::mem_fun(*this, &ChannelConfigWindow::on_cellrenderer_standard_edited) );

    // Column Standard:
    Gtk::TreeView::Column* pColumnStandard = Gtk::manage( new Gtk::TreeView::Column("Standard") ); 
    pColumnStandard->pack_start(*pRendererStandard);
    // Make this View column represent the m_col_standard model column:
    pColumnStandard->add_attribute(pRendererStandard->property_text(), m_Columns.m_col_standard);

    // --- Channel column ---

    // CellRendererCombo for column Channel:
    Gtk::CellRendererCombo* pRendererChannel = Gtk::manage( new Gtk::CellRendererCombo);
    // Use text from column 0 (m_col_choice) of m_refTreeModelComboChannel:
    pRendererChannel->property_text_column() = 0; 
    // Allow the user to edit the column:
    pRendererChannel->property_editable() = true;
    // Only allow to use values defined in combo box:
    pRendererChannel->property_has_entry() = false;
    pRendererChannel->signal_edited().connect( sigc::mem_fun(*this, &ChannelConfigWindow::on_cellrenderer_channel_edited) );

    // Column Channel:
    Gtk::TreeView::Column* pColumnChannel = Gtk::manage( new Gtk::TreeView::Column("Channel") ); 
    pColumnChannel->pack_start(*pRendererChannel);
    // Make this View column represent the m_col_channel model column:
    pColumnChannel->add_attribute(pRendererChannel->property_text(), m_Columns.m_col_channel);
    // Allow the user to choose from this list to set the value in m_col_channel:
    pColumnChannel->add_attribute(pRendererChannel->property_model(), m_Columns.m_col_channel_choices);

    // --- Fine Tune column ---

    // CellRendererSpin for column Finetune:
    Gtk::CellRendererSpin* pRendererFinetune = Gtk::manage( new Gtk::CellRendererSpin);
    // Allow the user to edit the column:
    pRendererFinetune->property_editable() = true;
    pRendererFinetune->property_adjustment() = &m_FinetuneAdjustment;
    pRendererFinetune->signal_edited().connect( sigc::mem_fun(*this, &ChannelConfigWindow::on_cellrenderer_finetune_edited) );

    // Columnt FineTune:
    Gtk::TreeView::Column* pColumnFinetune = Gtk::manage( new Gtk::TreeView::Column("Fine Tune") ); 
    pColumnFinetune->pack_start(*pRendererFinetune);
    pColumnFinetune->add_attribute(pRendererFinetune->property_text(), m_Columns.m_col_finetune);


    // --- Append all columns ---

    //Add the TreeView's view columns:
    m_TreeView.append_column_editable("Name", m_Columns.m_col_name);
    m_TreeView.append_column(*pColumnStandard);
    m_TreeView.append_column(*pColumnChannel);
    m_TreeView.append_column("Frequency", m_Columns.m_col_frequency);
    m_TreeView.append_column(*pColumnFinetune);
    m_TreeView.append_column("Signal", m_Columns.m_col_signal);

    // Allow rows to be drag and dropped within the treeview:
    m_TreeView.set_reorderable();

    // TreeView:
    //    Glib::SignalProxy2< void, const TreeModel::Path &, TreeViewColumn* > 	signal_row_activated ()

    // Context Menu
    // Create actions for menus and toolbars:
    m_refActionGroup = Gtk::ActionGroup::create();
    m_refActionGroup->add(Gtk::Action::create("TuneIntoChannel", "Tune Into Channel"),
                          sigc::mem_fun(*this, &ChannelConfigWindow::on_tune_channel));
    m_refActionGroup->add(Gtk::Action::create("AddEntryBefore", "Add Entry Before"),
                          sigc::mem_fun(*this, &ChannelConfigWindow::on_add_entry_before));
    m_refActionGroup->add(Gtk::Action::create("AddEntryAfter", "Add Entry After"),
                          sigc::mem_fun(*this, &ChannelConfigWindow::on_add_entry_after));
    m_refActionGroup->add(Gtk::Action::create("RemoveEntry", "Remove Entry"),
                          sigc::mem_fun(*this, &ChannelConfigWindow::on_remove_entry));
    m_refActionGroup->add(Gtk::Action::create("ScanChannels", "Scan Channels"),
                          sigc::mem_fun(*this, &ChannelConfigWindow::on_scan_channels));

    m_refUIManager = Gtk::UIManager::create();
    m_refUIManager->insert_action_group(m_refActionGroup);
    m_TreeView.add_events(Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);
    m_ScrolledWindow.add_events(Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);
    add_events(Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);

    Glib::ustring ui_info =
        "<ui>"
        "  <popup name='PopupMenu'>"
        "    <menuitem action='TuneIntoChannel'/>"
        "    <menuitem action='AddEntryBefore'/>"
        "    <menuitem action='AddEntryAfter'/>"
        "    <menuitem action='RemoveEntry'/>"
        "    <menuitem action='ScanChannels'/>"
        "  </popup>"
        "</ui>";

#ifdef GLIBMM_EXCEPTIONS_ENABLED
    try
    {
        m_refUIManager->add_ui_from_string(ui_info);
    }
    catch(const Glib::Error& ex)
    {
        std::cerr << "building menus failed: " <<  ex.what();
    }
#else
    std::auto_ptr<Glib::Error> ex;
    m_refUIManager->add_ui_from_string(ui_info, ex);
    if(ex.get())
    {
        std::cerr << "building menus failed: " <<  ex->what();
    }
#endif //GLIBMM_EXCEPTIONS_ENABLED

    // This only works with false. I.e. ChannelConfigWindow::on_button_press_event
    // gets the button press event before the default handler gets the event.
    m_TreeView.signal_button_press_event().connect(sigc::mem_fun(this, &ChannelConfigWindow::on_button_press_event), false);
    m_TreeView.signal_row_activated().connect(sigc::mem_fun(this, &ChannelConfigWindow::on_row_activated), false);

    m_refTreeModel->signal_row_changed().connect(sigc::mem_fun(this, &ChannelConfigWindow::on_row_changed), false);
    m_refTreeModel->signal_row_inserted().connect(sigc::mem_fun(this, &ChannelConfigWindow::on_row_inserted), false);
    m_refTreeModel->signal_row_deleted().connect(sigc::mem_fun(this, &ChannelConfigWindow::on_row_deleted), false);
    m_refTreeModel->signal_rows_reordered().connect(sigc::mem_fun(this, &ChannelConfigWindow::on_rows_reordered), false);

    show_all_children();
}

ChannelConfigWindow::~ChannelConfigWindow()
{
}

void ChannelConfigWindow::on_show_window(bool pshow)
{
    DEBUG();

    // This is the same as ControlWindow::on_show_control_window.

    if (pshow)
    {
        // Let the window manager decide where to place the window when shown
        // for the first time. Use previous position when shown again.
        if (m_shown)
        {
            // The window manager may ignore or modify the move request.
            // Partially visible windows are for example replaced by KDE.
            move(m_pos_x, m_pos_y);
        }
        show();
        m_shown = true;
    }
    else
    {
        get_position(m_pos_x, m_pos_y);
        hide();
    }
}

void ChannelConfigWindow::on_tuner_channel_tuned(const ChannelData& channelData)
{
    DEBUG();
    std::stringstream ss;
    ss << "Tuned " << channelData.standard
       << ", " << channelData.channel
       << ", " << channelData.getTunedFrequency()/1000.0 << " MHz";
    m_StatusBarMessage.set_text(ss.str());
}

void ChannelConfigWindow::on_tuner_signal_detected(const ChannelData& channelData)
{
    DEBUG();
    std::stringstream ss;
    ss << "Signal " << channelData.standard
       << ", " << channelData.channel
       << ", " << channelData.getTunedFrequency()/1000.0 << " MHz";
    m_StatusBarMessage.set_text(ss.str());

    TemporaryEnable e(dont_save);

    // Check if an entry for the cannel already exists:
    Gtk::TreeModel::Children children = m_refTreeModel->children();
    Gtk::TreeModel::iterator iter = children.begin();
    while (iter != children.end())
    {
	Gtk::TreeRow row = *iter;
	if (row[m_Columns.m_col_standard] == channelData.standard &&
	    row[m_Columns.m_col_channel] == channelData.channel)
	{
	    // Entry already exists
	    row[m_Columns.m_col_signal] = true;
	    return;
	}
	iter++;
    }

    // Add a new entry
    Gtk::TreeRow row = *(m_refTreeModel->append());
    row[m_Columns.m_col_name] = "";
    row[m_Columns.m_col_standard] = channelData.standard;
    row[m_Columns.m_col_channel] = channelData.channel;
    row[m_Columns.m_col_channel_choices] = m_ChannelListMap[channelData.standard];
    row[m_Columns.m_col_frequency] = channelData.frequency;
    row[m_Columns.m_col_finetune] = channelData.finetune;
    row[m_Columns.m_col_signal] = true;

    saveConfigurationData(true);
}

void ChannelConfigWindow::on_tuner_scan_stopped()
{
    DEBUG();
    m_StatusBarMessage.set_text("Interrupted scanning channels.");
}

void ChannelConfigWindow::on_tuner_scan_finished()
{
    DEBUG();
    m_StatusBarMessage.set_text("Scanning channels finished");
}

void ChannelConfigWindow::on_configuration_data_loaded(const ConfigurationData& configurationData)
{
    DEBUG();

    TemporaryEnable e(dont_save);

    const StationList& stationList = configurationData.stationList;
    StationList::const_iterator it = stationList.begin();
    while(it != stationList.end())
    {
	const StationData& stationData = *it;

	// Add a new entry
	Gtk::TreeRow row = *(m_refTreeModel->append());
	row[m_Columns.m_col_name] = stationData.name;
	row[m_Columns.m_col_standard] = stationData.standard;
	row[m_Columns.m_col_channel] = stationData.channel;
	row[m_Columns.m_col_channel_choices] = m_ChannelListMap[stationData.standard];
	updateFrequency(row);
	row[m_Columns.m_col_finetune] = stationData.fine;
	row[m_Columns.m_col_signal] = false;

	it++;
    }

    // This function receives saved configuration data, i.e. nothing to save here.
}

void ChannelConfigWindow::on_cellrenderer_standard_edited(
          const Glib::ustring& path_string,
	  const Glib::ustring& new_standard)
{
    DEBUG();

    Gtk::TreePath path(path_string);

    //Get the row from the path:
    Gtk::TreeModel::iterator iter = m_refTreeModel->get_iter(path);
    if(iter)
    {
	//Store the user's new text in the model:
	Gtk::TreeRow row = *iter;
	setStandard(row, new_standard);
    }
}

void ChannelConfigWindow::on_cellrenderer_channel_edited(
          const Glib::ustring& path_string,
	  const Glib::ustring& new_channel)
{
    DEBUG();

    Gtk::TreePath path(path_string);

    //Get the row from the path:
    Gtk::TreeModel::iterator iter = m_refTreeModel->get_iter(path);
    if(iter)
    {
	//Store the user's new text in the model:
	Gtk::TreeRow row = *iter;
	setChannel(row, new_channel);
    }
}

void ChannelConfigWindow::on_cellrenderer_finetune_edited(
          const Glib::ustring& path_string,
	  const Glib::ustring& new_text)
{
    DEBUG();

    Gtk::TreePath path(path_string);

    //Get the row from the path:
    Gtk::TreeModel::iterator iter = m_refTreeModel->get_iter(path);
    if(iter)
    {
	//Store the user's new text in the model:
	Gtk::TreeRow row = *iter;
	row[m_Columns.m_col_finetune] = atoi(new_text.c_str());
    }
}

void ChannelConfigWindow::on_row_activated (const Gtk::TreeModel::Path& path,
					    Gtk::TreeViewColumn* column)
{
    DEBUG();

    Gtk::TreeModel::iterator iter = m_refTreeModel->get_iter(path);
    if (iter)
    {
	Gtk::TreeRow row = *iter;
	tuneChannel(row);
    }
}

bool ChannelConfigWindow::on_button_press_event(GdkEventButton* event)
{
    if(event->type == GDK_BUTTON_PRESS)
    {
	if (event->button == 3)
        {
	    Gtk::TreeModel::Path path;
	    Gtk::TreeViewColumn* column;
	    int cell_x;
	    int cell_y; 
	    if (m_TreeView.get_path_at_pos(event->x, event->y, path, column, cell_x, cell_y ))
	    {
		int row_num = *path.get_indices().begin();
		// Gtk::TreeModel::Row row = *( m_refTreeModel->children[row_num - 1]);
		DEBUG( << "row_num = " << row_num );
	    }

            Gtk::Menu* popup = getPopupMenuWidget();
            if (popup)
            {
                popup->popup(event->button, event->time);
            }

	    // By returning false also the default handler gets the event:
            return false;
        }
    }
    if(event->type == GDK_2BUTTON_PRESS)
    {
	if (event->button == 1)
        {
	    DEBUG( << "Double Click" );
	    Glib::RefPtr<Gtk::TreeView::Selection> selection = m_TreeView.get_selection();
	    Gtk::TreeModel::iterator iter = selection->get_selected();
	    if (iter)
	    {
		Gtk::TreeModel::Row row = *iter;
		DEBUG( << "Selected: " << row[m_Columns.m_col_name] );
	    }
	}
    }

    // Event has not been handled:
    return false;
}

void ChannelConfigWindow::on_row_changed(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator&  iter)
{
    saveConfigurationData();
}

void ChannelConfigWindow::on_row_inserted(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter)
{
    saveConfigurationData();
}

void ChannelConfigWindow::on_row_deleted(const Gtk::TreeModel::Path& path)
{
    saveConfigurationData();
}

void ChannelConfigWindow::on_rows_reordered(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter, int* new_order)
{
    saveConfigurationData();
}

void ChannelConfigWindow::on_tune_channel()
{
    DEBUG();

    Glib::RefPtr<Gtk::TreeView::Selection> selection = m_TreeView.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (iter)
    {
	Gtk::TreeRow row = *iter;
	tuneChannel(row);
    }
}

void ChannelConfigWindow::on_add_entry_before()
{
    DEBUG();

    Glib::RefPtr<Gtk::TreeView::Selection> selection = m_TreeView.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (iter)
    {
	m_refTreeModel->insert(iter);
    }
    else
    {
	m_refTreeModel->append();
    }
}

void ChannelConfigWindow::on_add_entry_after()
{
    DEBUG();

    Glib::RefPtr<Gtk::TreeView::Selection> selection = m_TreeView.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (iter)
    {
	m_refTreeModel->insert_after(iter);
    }
    else
    {
	m_refTreeModel->append();
    }
}

void ChannelConfigWindow::on_remove_entry()
{
    DEBUG();

    Glib::RefPtr<Gtk::TreeView::Selection> selection = m_TreeView.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (iter)
    {
	m_refTreeModel->erase(iter);
    }
}

void ChannelConfigWindow::on_scan_channels()
{
    DEBUG();

    // Open dialog to select standard:
    ComboBoxDialog<Glib::ustring> dialog(m_refTreeModelComboStandard,
					 m_ColumnsCombo.m_col_choice,
					 "Scan", "Standard");
    if (! dialog.run())
    {
	return;
    }

    Glib::ustring standard = dialog.get_selected_value();

    if (!standard.empty())
    {
	// Setting all signal flags back to false:
	Gtk::TreeModel::Children children = m_refTreeModel->children();
	Gtk::TreeModel::iterator iter = children.begin();
	while (iter != children.end())
	{
	    TemporaryEnable e(dont_save);

	    Gtk::TreeRow row = *iter;
	    row[m_Columns.m_col_signal] = false;
	    iter++;
	}

	signalStartScan(standard);
    }
}

void ChannelConfigWindow::saveConfigurationData(bool force)
{
    if (!force && dont_save)
	return;

    std::cout << __PRETTY_FUNCTION__ << std::endl;

    ConfigurationData configurationData;
    StationList& stationList = configurationData.stationList;

    Gtk::TreeModel::Children children = m_refTreeModel->children();
    Gtk::TreeModel::iterator iter = children.begin();

    while (iter != children.end())
    {
	Gtk::TreeRow row = *iter;
	StationData stationData;
	stationData.name = Glib::ustring(row[m_Columns.m_col_name]);
	stationData.standard = Glib::ustring(row[m_Columns.m_col_standard]);
	stationData.channel = Glib::ustring(row[m_Columns.m_col_channel]);
	stationData.fine = row[m_Columns.m_col_finetune];
	stationList.push_back(stationData);
	iter++;
    }

    signalConfigurationDataChanged(configurationData);
}

void ChannelConfigWindow::setStandard(Gtk::TreeRow& row, const Glib::ustring& standard)
{
    if (row[m_Columns.m_col_standard] != standard)
    {
	TemporaryEnable e(dont_save);
	row[m_Columns.m_col_standard] = standard;
	row[m_Columns.m_col_channel_choices] = m_ChannelListMap[std::string(standard)];
	setChannel(row, "");
	saveConfigurationData(true);
    }
}

void ChannelConfigWindow::setChannel(Gtk::TreeRow& row, const Glib::ustring& channel)
{
    if (row[m_Columns.m_col_channel] != channel)
    {
	TemporaryEnable e(dont_save);
	row[m_Columns.m_col_channel] = channel;
	row[m_Columns.m_col_frequency] = 0;
	row[m_Columns.m_col_finetune] = 0; 
	row[m_Columns.m_col_signal] = false;
	updateFrequency(row);
	saveConfigurationData(true);
    }
}

void ChannelConfigWindow::updateFrequency(Gtk::TreeRow& row)
{
    Glib::ustring channel = row[m_Columns.m_col_channel];
    Glib::ustring standard = row[m_Columns.m_col_standard];
    ChannelFrequencyTable cft = ChannelFrequencyTable::create(standard.c_str());
    int ch = ChannelFrequencyTable::getChannelNumber(cft, channel.c_str());
    int freq = ChannelFrequencyTable::getChannelFreq(cft, ch);
    DEBUG(<< standard << ":" << channel << " = " << freq);
    if (row[m_Columns.m_col_frequency] != freq)
    {
	row[m_Columns.m_col_frequency] = freq;
	row[m_Columns.m_col_finetune] = 0;
	row[m_Columns.m_col_signal] = false;
    }
}

void ChannelConfigWindow::tuneChannel(const Gtk::TreeRow& row)
{
    ChannelData channelData;

    Glib::ustring ustandard = row[m_Columns.m_col_standard];
    Glib::ustring uchannel = row[m_Columns.m_col_channel];
    channelData.standard = ustandard;
    channelData.channel = uchannel;
    channelData.frequency = row[m_Columns.m_col_frequency];
    channelData.finetune = row[m_Columns.m_col_finetune];
    signalSetFrequency(channelData);
}

Gtk::Menu* ChannelConfigWindow::getPopupMenuWidget()
{
    return dynamic_cast<Gtk::Menu*>(m_refUIManager->get_widget("/PopupMenu"));
}
