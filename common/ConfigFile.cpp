//
// Config File
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

#include "common/ConfigFile.hpp"
#include "common/MediaCommon.hpp"
#include "common/GeneralEvents.hpp"
#include "platform/Logging.hpp"

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <sys/types.h>

#ifdef CONFIG_FILE_TEST
#undef TRACE_DEBUG
#define TRACE_DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;
#endif

namespace spirit = boost::spirit;
namespace karma = boost::spirit::karma;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;


// Adapt the structs to be a fully conforming fusion tuples:

BOOST_FUSION_ADAPT_STRUCT(StationData,
			  (std::string, name)
			  (std::string, standard)
			  (std::string, channel)
			  (int, fine));

BOOST_FUSION_ADAPT_STRUCT(ConfigurationGuiVisible,
			  (bool, menuBar)
			  (bool, toolBar)
			  (bool, statusBar));

BOOST_FUSION_ADAPT_STRUCT(ConfigurationGui,
			  (bool, fullscreen)
			  (ConfigurationGuiVisible, visibleWindow)
			  (ConfigurationGuiVisible, visibleFullscreen));

BOOST_FUSION_ADAPT_STRUCT(ConfigurationPlayer,
			  (bool, useOptimalPixelFormat)
			  (bool, useXvClipping)
			  (std::string, deinterlacer));

BOOST_FUSION_ADAPT_STRUCT(ConfigurationData,
			  (StationList, stationList)
			  (ConfigurationGui, configGui)
			  (ConfigurationPlayer, configPlayer));

// For debugging:

std::ostream& operator<<(std::ostream& strm, const StationData& s)
{
    strm << "name = " << s.name << std::endl;
    strm << "standard = " << s.standard << std::endl;
    strm << "channel = " << s.channel << std::endl;
    strm << "fine = " << s.fine << std::endl;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const StationList& l)
{
    for (StationList::const_iterator it = l.begin(); it != l.end(); it++)
    {
	strm << *it << std::endl;
    }

    return strm;
}

std::ostream& operator<<(std::ostream& strm, const ConfigurationGuiVisible& cgv)
{
    strm << "menuBar = " << cgv.menuBar << std::endl;
    strm << "toolBar = " << cgv.toolBar << std::endl;
    strm << "statusBar = " << cgv.statusBar << std::endl;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const ConfigurationGui& cg)
{
    strm << "fullscreen = " << cg.fullscreen << std::endl;
    strm << "visibleWindow = " << cg.visibleWindow << std::endl;
    strm << "visibleFullscreen = " << cg.visibleFullscreen << std::endl;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const ConfigurationPlayer& cp)
{
    strm << "useOptimalPixelFormat = " << cp.useOptimalPixelFormat << std::endl;
    strm << "useXvClipping = " << cp.useXvClipping << std::endl;
    strm << "deinterlacer = " << cp.deinterlacer << std::endl;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const ConfigurationData& cnf)
{
    strm << cnf.stationList << std::endl;
    strm << cnf.configGui << std::endl;
    strm << cnf.configPlayer << std::endl;
    return strm;
}

// Semantic actions (for debugging):
void actionStationName(std::string const& s) {TRACE_DEBUG( << s);}
void actionStationStandard(std::string const& s) {TRACE_DEBUG( << s);}
void actionStationChannel(std::string const& s) {TRACE_DEBUG( << s);}
void actionStationFine(int const& i) {TRACE_DEBUG( << i);}
void actionStation(StationData const& stationData){TRACE_DEBUG(<< std::endl << stationData);}
void actionGui(bool b) {TRACE_DEBUG(<< b);}
void actionPlayer(bool b) {TRACE_DEBUG(<< b);}

// Configuration File Parser:

template <typename ForwardIterator>
struct config_parser : qi::grammar<ForwardIterator, ConfigurationData(), ascii::space_type>
{
    config_parser() : config_parser::base_type(config_data_)
    {
        using qi::int_;
        using qi::lit;
        using boost::spirit::qi::lexeme;
        using ascii::char_;
	using qi::bool_;

        quoted_string %= lexeme['"' >> *(char_ - '"') >> '"'];

        station_ %= lit("station")
            >> '{'
            >> ( ( lit("name") >> '=' >> quoted_string >> ';' ) ^
		 ( lit("standard") >> '=' >> quoted_string >> ';' ) ^
		 ( lit("channel") >> '=' >> quoted_string >> ';' ) ^
		 ( lit("fine") >> '=' >> int_ >> ';' ) )
            >> '}' >> ";";

        station_list_ %= *(station_ /* [&actionStation] */);

	config_gui_visible_ %= lit("{")
	    >> ( ( lit("menuBar") >> '=' >> bool_ >> ';' ) ^
		 ( lit("toolBar") >> '=' >> bool_ >> ';' ) ^
		 ( lit("statusBar") >> '=' >> bool_ >> ';' ) )
		 >> '}';

	config_gui_ %= lit("gui")
            >> '{'
            >> ( ( lit("fullscreen") >> '=' >> bool_ >> ';' ) ^
		 ( lit("window") >> config_gui_visible_ >> ';') ^
		 ( lit("fullscreen") >> config_gui_visible_ >> ';') )
            >> '}' >> ";";

	config_player_ %= lit("player")
            >> '{'
            >> ( ( lit("useOptimalPixelFormat") >> '=' >> bool_ >> ';' ) ^
		 ( lit("useXvClipping") >> '=' >> bool_ >> ';' ) ^
		 ( lit("deinterlacer") >> '=' >> quoted_string >> ';' ) )
            >> '}' >> ";";

	config_data_ %=
	    station_list_ ^
	    config_gui_ ^
	    config_player_;
    }

    qi::rule<ForwardIterator, std::string(), ascii::space_type> quoted_string;
    qi::rule<ForwardIterator, StationData(), ascii::space_type> station_;
    qi::rule<ForwardIterator, StationList(), ascii::space_type> station_list_;
    qi::rule<ForwardIterator, ConfigurationGuiVisible(), ascii::space_type> config_gui_visible_;
    qi::rule<ForwardIterator, ConfigurationGui(), ascii::space_type> config_gui_;
    qi::rule<ForwardIterator, ConfigurationPlayer(), ascii::space_type> config_player_;

    qi::rule<ForwardIterator, ConfigurationData(), ascii::space_type> config_data_;
};

template <typename OutputIterator>
struct config_generator : karma::grammar<OutputIterator, ConfigurationData()>
{
    config_generator() : config_generator::base_type(config_data_)
    {
        using karma::int_;
        using karma::lit;
	using karma::bool_;

        quoted_string = '"' << karma::string << '"';

        station_ = lit("station")
            <<  " { "
            <<  lit("name") << " = " << quoted_string << "; "
            <<  lit("standard") << " = " << quoted_string << "; "
            <<  lit("channel") << " = " << quoted_string << "; "
            <<  lit("fine") << " = " << int_ << "; "
            <<  "};\n";

	station_list_ = *(station_);

	config_gui_visible_ = lit("\n    {\n")
	    << lit("        menuBar") << " = " << bool_ << ";\n"
	    << lit("        toolBar") << " = " << bool_ << ";\n"
	    << lit("        statusBar") << " = " << bool_ << ";\n"
	    << "    }";

	config_gui_ = lit("\ngui\n")
            << "{\n"
            << lit("    fullscreen") << " = " << bool_ << ";\n"
	    << lit("    window") << config_gui_visible_ << ";\n"
	    << lit("    fullscreen") << config_gui_visible_ << ";\n"
            << "};\n";

	config_player_ = lit("\nplayer\n")
            << "{\n"
            << lit("    useOptimalPixelFormat") << " = " << bool_ << ";\n"
	    << lit("    useXvClipping") << " = " << bool_ << ";\n"
	    << lit("    deinterlacer") << " = " << quoted_string << ";\n"
            << "};\n";

	config_data_ =
	    station_list_ <<
	    config_gui_ <<
	    config_player_;
    }

    karma::rule<OutputIterator, std::string()> quoted_string;
    karma::rule<OutputIterator, StationData()> station_;
    karma::rule<OutputIterator, StationList()> station_list_;
    karma::rule<OutputIterator, ConfigurationGuiVisible()> config_gui_visible_;
    karma::rule<OutputIterator, ConfigurationGui()> config_gui_;
    karma::rule<OutputIterator, ConfigurationPlayer()> config_player_;

    karma::rule<OutputIterator, ConfigurationData()> config_data_;

};

void ConfigFile::parse()
{
    if (!find())
    {
	// Config file does not yet exists.
	return;
    }

    // Opening input file:
    std::ifstream in(fileName.c_str());
    if (!in.is_open())
    {
	TRACE_ERROR( << "Opening config file \'" << fileName << "\' failed.");
	return;
    }

    // Parse:
    // Spirit needs a forward iterator, std::istream_iterator<> is an input iterator only.
    typedef spirit::basic_istream_iterator<char> base_iterator_type;
    base_iterator_type begin(in);
    base_iterator_type end;

    boost::shared_ptr<ConfigurationData> configurationData(new ConfigurationData());
    ConfigurationData& confData = *configurationData;

    if (! qi::phrase_parse(begin, end, config_parser<base_iterator_type>(), ascii::space, confData))
    {
	TRACE_ERROR( << "parse failed");
	return;
    }

    if (begin != end)
    {
	TRACE_ERROR( << "parse incomplete");
	return;
    }

    TRACE_DEBUG(<< confData);

    // Notify application about the read configuration:
    mediaCommon->queue_event(configurationData);
}

void ConfigFile::generate(ConfigurationData& configurationData)
{
    find();

    // Opening output file:

#ifndef CONFIG_FILE_TEST
    std::ofstream out(fileName.c_str());
#else
    std::ofstream out((fileName+"out").c_str());
#endif
    if (!out.is_open())
    {
	TRACE_ERROR( << "Opening config file \'" << fileName << "\' failed.");
	return;
    }

    // Generate:

    TRACE_DEBUG(<< configurationData.configPlayer.deinterlacer);

    typedef std::ostream_iterator<char> sink_type;
    sink_type sink(out);
    if (! karma::generate(sink, config_generator<sink_type>(), configurationData))
    {
	TRACE_ERROR( << "generate failed");
	return;
    }

    mediaCommon->queue_event(boost::make_shared<ConfigurationFileWritten>());
}

bool ConfigFile::find()
{
    // 1st: Use playerrc in working directory if it exists:
    std::string fn("playerrc");
    boost::filesystem::path p(fn);
    if (boost::filesystem::is_regular_file(p))
    {
	fileName = fn;
	return true;
    }
    
    // 2nd: Use ~/.playerrc, create it if it does not exist:
    char* home = std::getenv("HOME");
    if (home)
    {
	fileName = std::string(home).append("/.playerrc");
	boost::filesystem::path p(fileName);
	return boost::filesystem::is_regular_file(p);
    }

    // 3rd: Fallback to playerrc in working directory:
    fileName = fn;
    return false;
}

void ConfigFile::process(boost::shared_ptr<CommonInitEvent> event)
{
    TRACE_DEBUG(<< "tid = " << gettid());
    mediaCommon = event->mediaCommon;
    parse();
}

void ConfigFile::process(boost::shared_ptr<ConfigurationData> configurationData)
{
    generate(*configurationData);
}
