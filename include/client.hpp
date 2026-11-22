#ifndef CLIENT
# define CLIENT
# include "include.hpp"

class Client
{
public:
	int			fd;
	std::string	hostname;
	std::string	nickname;
	std::string	username;
	std::string	inbuffer;	// ce qu'on a recu
	std::string	outbuffer;	// ce qu'il reste a envoyer
	bool		password_given;
	bool		registered;

	Client(int fd, std::string hostname);
	int		read_data();
	int		write_data();
	bool	get_line(std::string &line);
};

#endif
