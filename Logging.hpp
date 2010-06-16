#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <iostream>

#define DEBUG(x)
// #define DEBUG(x) std::cout << __PRETTY_FUNCTION__  x << std::endl
#define ERROR(x) std::cerr << "Error: " << __PRETTY_FUNCTION__  x << std::endl

#endif
