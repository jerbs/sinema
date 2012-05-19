//
// About and Help dialog
//
// Copyright (C) Joachim Erbs, 2010-2012
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

#include "gui/About.hpp"
#include "gui/General.hpp"
#include "gui/COPYING.inc"
#include "gui/README.inc"
#include "gui/version.inc"
// #include "gui/logo.xpm"

const std::string applicationName("sinema");

// ===================================================================

AboutDialog::AboutDialog()
{
    std::string copying(COPYING, sizeof(COPYING));
    std::string version(::version, sizeof(::version));
    set_program_name(applicationName);
    set_version(version);
    set_comments("A Media Player and TV Viewer");
    set_copyright("Copyright (C) Joachim Erbs, 2009-2012");
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
    using std::string;

    set_title(applicationName + " (Help)");
    set_default_size(520,1500);

    string readme(README, sizeof(README));

    m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    m_refTextBuffer = Gtk::TextBuffer::create();

    // line starts with 2 or more spaces
    TextTagPtr tagHeading1 = m_refTextBuffer->create_tag("heading1"); 
    tagHeading1->property_weight() = PANGO_WEIGHT_BOLD;
    tagHeading1->property_justification() = Gtk::JUSTIFY_CENTER;
    tagHeading1->property_size() = 18 * Pango::SCALE;

    // line starts with 2 or more spaces and ends with a tab
    TextTagPtr tagHeading2 = m_refTextBuffer->create_tag("heading2"); 
    tagHeading2->property_weight() = PANGO_WEIGHT_BOLD;
    tagHeading2->property_justification() = Gtk::JUSTIFY_CENTER;
    tagHeading2->property_size() = 14 * Pango::SCALE;

    // line ends with a tab
    TextTagPtr tagHeading3 = m_refTextBuffer->create_tag("heading3"); 
    tagHeading3->property_weight() = PANGO_WEIGHT_BOLD;
    tagHeading3->property_size() = 14 * Pango::SCALE;

    TextTagPtr tagBody = m_refTextBuffer->create_tag("body"); 
    tagBody->property_size() = 10 * Pango::SCALE;
    tagBody->property_wrap_mode() = Gtk::WrapMode::WRAP_WORD;

    // line contains 2 subsequent spaces
    TextTagPtr tagNoWrap = m_refTextBuffer->create_tag("nowrap"); 
    tagNoWrap->property_font() = "Courier";
    tagNoWrap->property_size() = 10 * Pango::SCALE;
    tagNoWrap->property_wrap_mode() = Gtk::WrapMode::WRAP_NONE;

    Gtk::TextBuffer::iterator it = m_refTextBuffer->end();

    string::size_type sol = 0;
    string paragraph;

    while(sol < readme.size())
    {
	string::size_type eol = readme.find_first_of('\n', sol);
	if (eol == string::npos) eol = readme.size();

	string line(readme, sol, eol-sol);

	if (line.empty())
	{
	    if (!paragraph.empty())
	    {
		paragraph += "\n";
		it = m_refTextBuffer->insert_with_tag(it, paragraph, tagBody);
		paragraph.clear();
	    }

	    it = m_refTextBuffer->insert_with_tag(it, "\n", tagBody);
	}
	else
	{
	    string::size_type posDoubleSpace = line.find("  ");

	    bool tab = (line[line.size()-1] == '\t');
	    if (tab) line.erase(line.size()-1);

	    if (posDoubleSpace == 0)
	    {
		line.erase(0, line.find_first_not_of(' '));
		line += "\n";
		if (!tab)
		{
		    it = m_refTextBuffer->insert_with_tag(it, line, tagHeading1);
		}
		else
		{
		    it = m_refTextBuffer->insert_with_tag(it, line, tagHeading2);
		}
	    }
	    else if (tab)
	    {
		line += "\n";
		it = m_refTextBuffer->insert_with_tag(it, line, tagHeading3);
	    }
	    else if (posDoubleSpace != string::npos)
	    {
		line += "\n";
		it = m_refTextBuffer->insert_with_tag(it, line, tagNoWrap);
	    }
	    else
	    {
		if (!paragraph.empty()) paragraph += " ";
		paragraph += line;
	    }
	}

	sol = eol + 1;
    }

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
