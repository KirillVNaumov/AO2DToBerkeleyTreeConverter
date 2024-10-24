#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <cassert>
#include <iostream>

#ifdef NDEBUG
#define DEBUG(x) 
#else
#define DEBUG(x) do { std::cerr << x << std::endl; } while (0);
#endif

#endif
