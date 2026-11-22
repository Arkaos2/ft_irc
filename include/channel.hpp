#ifndef CHANNEL
# define CHANNEL
# include "include.hpp"

// un channel garde juste les fd de ses membres et de ses operateurs
class Channel
{
public:
	std::set<int>	members;
	std::set<int>	ops;
};

#endif
