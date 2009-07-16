#ifndef GENERAL_EVENTS_HPP
#define GENERAL_EVENTS_HPP

#include <iostream>

#define DEBUG(x) std::cout << __PRETTY_FUNCTION__  x << std::endl

struct Quit
{
    Quit(){DEBUG();}
    ~Quit(){DEBUG();}
};

struct Start
{
    Start(){DEBUG();}
    ~Start(){DEBUG();}
};

struct Stop
{
    Stop(){DEBUG();}
    ~Stop(){DEBUG();}
};

#endif
