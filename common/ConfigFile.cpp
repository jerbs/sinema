//
// Config File
//
// Copyright (C) Joachim Erbs, 2010
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

namespace spirit = boost::spirit;
namespace karma = boost::spirit::karma;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

typedef StationData station;
typedef StationList station_list;

#if 0
struct station
{
    std::string name;
    std::string standard;
    std::string channel;
    int fine;
};
#endif

// Adapt the struct to be a fully conforming fusion tuple:

BOOST_FUSION_ADAPT_STRUCT(
    station,
    (std::string, name)
    (std::string, standard)
    (std::string, channel)
    (int, fine)
)

template <typename ForwardIterator>
struct config_parser : qi::grammar<ForwardIterator, station_list(), ascii::space_type>
{
    config_parser() : config_parser::base_type(station_list_)
    {
        using qi::int_;
        using qi::lit;
        using qi::lexeme;
        using ascii::char_;

        quoted_string %= lexeme['"' >> *(char_ - '"') >> '"'];

        station_ %= lit("station")
            >> '{'
            >>  lit("name") >> '=' >> quoted_string >> ';'
            >>  lit("standard") >> '=' >> quoted_string >> ';'
            >>  lit("channel") >> '=' >> quoted_string >> ';'
            >>  lit("fine") >> '=' >> int_ >> ';'
            >>  '}' >> ";";

        station_list_ %= *(station_);
    }

    qi::rule<ForwardIterator, std::string(), ascii::space_type> quoted_string;
    qi::rule<ForwardIterator, station(), ascii::space_type> station_;
    qi::rule<ForwardIterator, station_list(), ascii::space_type> station_list_;
};

template <typename OutputIterator>
struct config_generator : karma::grammar<OutputIterator, station_list()>
{
    config_generator() : config_generator::base_type(station_list_)
    {
        using karma::int_;
        using karma::lit;

        quoted_string = '"' << karma::string << '"';

        station_ = lit("station")
            <<  " { "
            <<  lit("name") << " = " << quoted_string << "; "
            <<  lit("standard") << " = " << quoted_string << "; "
            <<  lit("channel") << " = " << quoted_string << "; "
            <<  lit("fine") << " = " << int_ << "; "
            <<  "};\n";

	station_list_ = *(station_);
    }

    karma::rule<OutputIterator, std::string()> quoted_string;
    karma::rule<OutputIterator, station()> station_;
    karma::rule<OutputIterator, station_list()> station_list_;
};


std::ostream& operator<<(std::ostream& strm, const station& s)
{
    strm << "name = " << s.name << std::endl;
    strm << "standard = " << s.standard << std::endl;
    strm << "channel = " << s.channel << std::endl;
    strm << "fine = " << s.fine << std::endl;
    return strm;
}

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
	ERROR( << "Opening config file \'" << fileName << "\' failed.");
	return;
    }

    // Parse:
    // Spirit needs a forward iterator, std::istream_iterator<> is an input iterator only.
    typedef spirit::basic_istream_iterator<char> base_iterator_type;
    base_iterator_type begin(in);
    base_iterator_type end;

    boost::shared_ptr<ConfigurationData> configurationData(new ConfigurationData());
    StationList& stationList = configurationData->stationList;

    if (! qi::phrase_parse(begin, end, config_parser<base_iterator_type>(), ascii::space, stationList))
    {
	ERROR( << "parse failed");
	return;
    }

    if (begin != end)
    {
	ERROR( << "parse incomplete");
	return;
    }

    // Notify application about the read configuration:
    mediaCommon->queue_event(configurationData);
}

void ConfigFile::generate(ConfigurationData& configurationData)
{
    StationList& stationList = configurationData.stationList;

    find();

    // Opening output file:

    std::ofstream out(fileName.c_str());
    if (!out.is_open())
    {
	ERROR( << "Opening config file \'" << fileName << "\' failed.");
	return;
    }

    // Generate:

    typedef std::ostream_iterator<char> sink_type;
    sink_type sink(out);
    if (! karma::generate(sink, config_generator<sink_type>(), stationList))
    {
	ERROR( << "generate failed");
	return;
    }
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
    mediaCommon = event->mediaCommon;
    parse();
}

void ConfigFile::process(boost::shared_ptr<ConfigurationData> configurationData)
{
    generate(*configurationData);
}
