//
// Config File
//
// Copyright (C) Joachim Erbs, 2010
//

#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <iostream>
#include <vector>

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

template <typename Iterator>
struct config_parser : qi::grammar<Iterator, station_list(), ascii::space_type>
{
    config_parser() : config_parser::base_type(start)
    {
        using qi::int_;
        using qi::lit;
        using qi::lexeme;
        using ascii::char_;

        quoted_string %= lexeme['"' >> +(char_ - '"') >> '"'];

        start %=
            *(
            lit("station")
            >> '{'
            >>  lit("name") >> '=' >> quoted_string >> ';'
            >>  lit("standard") >> '=' >> quoted_string >> ';'
            >>  lit("channel") >> '=' >> quoted_string >> ';'
            >>  lit("fine") >> '=' >> int_ >> ';'
            >>  '}' >> ";"
	    )
            ;
    }

    qi::rule<Iterator, std::string(), ascii::space_type> quoted_string;
    qi::rule<Iterator, station_list(), ascii::space_type> start;
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
    std::string input("station { name = \"ARD\"; standard = \"europe-west\"; channel = \"E5\"; fine = 0; };"
		      "station { name = \"ZDF\"; standard = \"europe-west\"; channel = \"E9\"; fine = 0; };");
    std::string::iterator begin = input.begin();
    std::string::iterator end = input.end();

    station_list result;

    if (! qi::phrase_parse(begin, end, config_parser<std::string::iterator>(), ascii::space, result))
    {
	std::cout << "parse failed" << std::endl;
	return -1;
    }

    if (begin != end)
    {
	std::cout << "parse incomplete" << std::endl;
	return -1;
    }

    station_list::iterator it = result.begin();

    while(it != result.end())
    {
        std::cout << "----" << std::endl << *it << std::endl;
	it++;
    }

    return 0;
}

#endif
