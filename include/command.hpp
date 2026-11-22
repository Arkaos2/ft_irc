#ifndef COMMAND
# define COMMAND
# include "include.hpp"

// Une ligne "NICK bob" ou "PRIVMSG #chan :salut a tous"
// name = "PRIVMSG", params = {"#chan", "salut a tous"}
class Command
{
public:
	std::string					name;
	std::vector<std::string>	params;

	Command(const std::string &line);
};

#endif
