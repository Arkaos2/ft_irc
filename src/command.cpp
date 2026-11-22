#include "../include/command.hpp"

Command::Command(const std::string &line)
{
	std::istringstream	iss(line);
	std::string			word;

	iss >> this->name;
	for (size_t i = 0; i < this->name.size(); i++)
		this->name[i] = std::toupper(this->name[i]);
	while (iss >> word)
	{
		// un mot qui commence par ':' prend tout le reste de la ligne
		if (word[0] == ':')
		{
			std::string rest;
			std::getline(iss, rest);
			this->params.push_back(word.substr(1) + rest);
			return ;
		}
		this->params.push_back(word);
	}
}
