#include "include/server.hpp"

static void	handle_signal(int)
{
	g_stop = 1;
}

static bool	valid_port(const std::string &port)
{
	if (port.empty() || port.size() > 5)
		return (false);
	for (size_t i = 0; i < port.size(); i++)
		if (!std::isdigit(port[i]))
			return (false);
	int	nb = std::atoi(port.c_str());
	return (nb >= 1 && nb <= 65535);
}

// cree la socket d'ecoute, retourne -1 en cas d'erreur
static int	create_socket(int port)
{
	int			fd = socket(AF_INET, SOCK_STREAM, 0);
	int			opt = 1;
	sockaddr_in	addr;

	if (fd < 0)
		return (-1);
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
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
	if (argc != 3 || !valid_port(argv[1]) || argv[2][0] == '\0')
	{
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return (1);
	}
	std::signal(SIGINT, handle_signal);
	std::signal(SIGTERM, handle_signal);
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
