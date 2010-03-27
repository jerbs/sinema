//
// Config File
//
// Copyright (C) Joachim Erbs, 2010
//

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <iostream>
#include <fstream>
#include <vector>

namespace spirit = boost::spirit;
namespace karma = boost::spirit::karma;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

struct station
{
    std::string name;
    std::string standard;
    std::string channel;
    int fine;
};

typedef std::vector<station> station_list;

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

        quoted_string %= lexeme['"' >> +(char_ - '"') >> '"'];

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

#ifdef TEST

int main()
{
    // Opening input file:

    std::ifstream in("example.conf");
    if (!in.is_open())
    {
	std::cout << "Opening input file failed." << std::endl;
	return -1;
    }

    // Parse:

    // Spirit needs a forward iterator, std::istream_iterator<> is an input iterator only.
    typedef spirit::basic_istream_iterator<char> base_iterator_type;
    base_iterator_type begin(in);
    base_iterator_type end;

    station_list result;

    if (! qi::phrase_parse(begin, end, config_parser<base_iterator_type>(), ascii::space, result))
    {
	std::cout << "parse failed" << std::endl;
	return -1;
    }

    if (begin != end)
    {
	std::cout << "parse incomplete" << std::endl;
	return -1;
    }

    // Print:

    station_list::iterator it = result.begin();

    while(it != result.end())
    {
        std::cout << "----" << std::endl << *it << std::endl;
	it++;
    }

    // Opening output file:

    std::ofstream out("output.conf");
    if (!out.is_open())
    {
	std::cout << "Opening output file failed." << std::endl;
	return -1;
    }

    // Generate:

    typedef std::ostream_iterator<char> sink_type;
    sink_type sink(out);

    if (! karma::generate(sink, config_generator<sink_type>(), result))
    {
	std::cout << "generate failed" << std::endl;
	return -1;
    }

    return 0;
}

#endif
