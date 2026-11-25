#include "include/server.hpp"

static void	handle_signal(int)
{
	g_stop = 1;
}

// cree la socket d'ecoute, retourne -1 en cas d'erreur
static int	create_socket(int port)
{
	int			fd = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in	addr;

	if (fd < 0)
		return (-1);
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	if (bind(fd, (sockaddr *)&addr, sizeof(addr)) < 0 || listen(fd, SOMAXCONN) < 0)
	{
		close(fd);
		return (-1);
	}
	return (fd);
}

int	main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return (1);
	}
	std::signal(SIGINT, handle_signal);
	std::signal(SIGTERM, handle_signal);
	// sinon send() sur un client deconnecte tue le serveur
	std::signal(SIGPIPE, SIG_IGN);
	int	fd = create_socket(std::atoi(argv[1]));
	if (fd < 0)
	{
		std::cerr << "Error: cannot start server on this port" << std::endl;
		return (1);
	}
	Server	server(fd, argv[2]);
	server.loop();
	return (0);
}
