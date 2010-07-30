//
//  wrap, a resource file wrapper for C/C++ 
//
//  Copyright (C) 2010 Joachim Erbs <joachim.erbs@gmx.de>
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <iostream>
#include <fstream>

#include "platform/Logging.hpp"

using namespace std;

int main(int argc, char* argv[])
{
    const char* inputFileName = argv[1];
    const char* outputFileName = argv[2];

    if (argc != 3)
    {
	std::cout << "Usage: wrap <input file> <output file>" << std::endl;
	exit(-1);
    }

    std::string ifn(inputFileName);
    string::size_type beg = ifn.find_last_of('/');
    if (beg == string::npos) beg = 0; else beg++;
    string::size_type end = ifn.find_first_of('.', beg);
    if (beg == string::npos) end = ifn.size();

    std::string symbolName(inputFileName, beg, end-beg);

    ifstream ifs(inputFileName);
    ofstream ofs(outputFileName);

    ofs << "const char " << symbolName << "[] = {" << std::endl;

    int col = 0;
    while(ifs.good())
    {
	char ch;
	ifs.get(ch);
	if (ifs.good())
	{
	    if (col == 0)
	    {
		ofs << "    ";
	    }

	    ofs << "0x" << hexDump(&ch, 1) << ",";

	    if (col == 15)
	    {
		ofs << std::endl; col = 0;
	    }
	    else
	    {
		col++;
	    }
	}
    }

    ofs << "};" << std::endl;
}
