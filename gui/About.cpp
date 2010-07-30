//
// About and Help dialog
//
// Copyright (C) Joachim Erbs, 2010
//

#include "gui/About.hpp"
#include "gui/General.hpp"
#include "gui/COPYING.inc"
// #include "gui/logo.xpm"

const std::string applicationName("sinema");

// ===================================================================

AboutDialog::AboutDialog()
{
    std::string copying(&COPYING[0], sizeof(COPYING));
    set_program_name(applicationName);
    set_version("0.0.0");
    set_comments("A Media Player and TV Viewer");
    set_copyright("Copyright (C) Joachim Erbs, 2009-2010");
    // set_website("");
    set_license(copying);
    std::list<Glib::ustring> authors;
    authors.push_back("Joachim Erbs <joachim.erbs@gmx.de>");
    set_authors(authors);
    // set_logo(Gdk::Pixbuf::create_from_xpm_data(....));
    set_wrap_license(false);
}

AboutDialog::~AboutDialog()
{}

void AboutDialog::on_response (int)
{
    TRACE_DEBUG();
    hide();
}

// ===================================================================

HelpDialog::HelpDialog()
{
    set_title(applicationName + " (Help)");
    set_default_size(520,1500);

    m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    m_refTextBuffer = Gtk::TextBuffer::create();

    TextTagPtr tagHeading1 = m_refTextBuffer->create_tag("heading1"); 
    tagHeading1->property_weight() = PANGO_WEIGHT_BOLD;
    tagHeading1->property_size() = 20 * Pango::SCALE;

    TextTagPtr tagHeading2 = m_refTextBuffer->create_tag("heading2"); 
    tagHeading2->property_weight() = PANGO_WEIGHT_BOLD;
    tagHeading2->property_size() = 15 * Pango::SCALE;

    TextTagPtr tagBody = m_refTextBuffer->create_tag("body"); 
    tagBody->property_font() = "Courier";
    tagBody->property_size() = 10 * Pango::SCALE;
    tagBody->property_wrap_mode() = Gtk::WrapMode::WRAP_WORD;

    TextTagPtr tagNoWrap = m_refTextBuffer->create_tag("nowrap"); 
    tagNoWrap->property_font() = "Courier";
    tagNoWrap->property_size() = 10 * Pango::SCALE;
    tagNoWrap->property_wrap_mode() = Gtk::WrapMode::WRAP_NONE;


    std::string description = applicationName + " is a free, GPL licensed "
	"media player and TV application.\n"
	"\n"
	"FFmpeg is used for video and audio decoding. " + applicationName + " "
	"can play every format supported by the installed FFmpeg library.\n"
	"\n"
	"ALSA is used to access the sound card. Accessing ALSA directly is "
	"possible from a single application only. Usually this is not wanted. "
	"It can be avoided by using Pulse Audio with a virtual ALSA device. "
	"Using Pulse Audio directly is not supported.\n"
	"\n"
	"XVideo is the only supported video output interface.\n"
	"\n"
	"TV viewing is only supported with analog TV cards having an "
	"MPEG hardware encoder. It is expected, that the encoded MPEG data is "
	"available at /dev/video0. The command \"cp /dev/video0 tv.mpg\" should "
	"produce a valid MPEG file. Otherwise watching TV will not be possible "
	"with " + applicationName + ". Watching TV is tested with the "
	"IVTV driver. See <http://ivtvdriver.org> for details about supported "
	"TV cards.\n"
	"\n"
	"The time shifting implementation is still very basic. Live TV is "
	"written to /tmp/tv.mpg and simultaneously that file is played. The "
	"file name is hard-coded. During a TV session that file "
	"isn\'t truncated. Ensure that the partition at /tmp provides enough "
	"free space. \n"
	"\n"
	"Cropping the video picture is possible to avoid black borders. Some "
	"cropping areas are predefined and can be enabled via the View menu. "
	"Alternatively the cropping area can intuitively be selected using the "
	"mouse. Move the mouse pointer to the edges and press the right mouse "
	"button. The shape of the mouse pointer indicates the action. Pressing "
	"the right mouse button, when the mouse is placed in the middle of the "
	"video picture, undoes any cropping.\n"
	"\n"
	"Pressing the middle mouse button shows and hides an additional control "
	"window. Pressing the right mouse button opens a context menu with the "
	"most important items also available in the main menu. This is especially "
	"useful for the full screen mode.\n" 	
	"\n";

    std::string hotkeyDescr = 
	"Cursor-Left                  Jump 10 seconds back\n"
	"Cursor-Right                 Jump 10 seconds forward\n"
	"Shift + Cursor-Left          Jump 30 seconds back\n"
	"Shift + Cursor-Right         Jump 30 seconds forward\n"
	"Ctrl + Cursor-Left           Jump 60 seconds back\n"
	"Ctrl + Cursor-Right          Jump 60 seconds forward\n"
	"Ctrl + Shift + Cursor-Left   Jump 180 seconds back\n"
	"Ctrl + Shift + Cursor-Right  Jump 180 seconds forward\n"
	"Space                        Toggle play/pause\n"
	"Page Up                      Skip to previous play list entry\n"
	"Page Down                    Skip to next play list entry\n"
	"Cursor-Up                    Increase volume\n"
	"Cursor-Down                  Decrease volume\n"
	"m                            Toggle mute mode\n"
	"Escape                       Leave full screen mode\n";

    Gtk::TextBuffer::iterator it = m_refTextBuffer->end();

    it = m_refTextBuffer->insert_with_tag(it, applicationName + "\n\n", tagHeading2);
    it = m_refTextBuffer->insert_with_tag(it, description, tagBody);

    it = m_refTextBuffer->insert_with_tag(it, "Hotkeys\n\n", tagHeading2);
    it = m_refTextBuffer->insert_with_tag(it, hotkeyDescr, tagNoWrap);

    m_TextView.set_buffer(m_refTextBuffer);
    m_TextView.set_editable(false);
    m_TextView.set_cursor_visible(false);

    m_ScrolledWindow.add(m_TextView);
    add(m_ScrolledWindow);

    show_all_children();
}

HelpDialog::~HelpDialog()
{}

void HelpDialog::on_response (int)
{
    TRACE_DEBUG();
    hide();
}

// ===================================================================

