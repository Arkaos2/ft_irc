#ifndef SERVER
# define SERVER
# include "include.hpp"
# include "client.hpp"
# include "command.hpp"
# include "channel.hpp"

class Server
{
private:
	std::string						password;
	int								listen_fd;
	std::vector<pollfd>				all_fd;
	std::map<int, Client>			all_client;
	std::map<std::string, Channel>	all_channel;	

	void	accept_client();
	void	remove_client(size_t i);
	bool	handle_client(size_t i);
	void	send_channel(const std::string &name, const std::string &msg, int except_fd);

	void	execute(Client &client, const std::string &line);
	void	try_register(Client &client);
	void	cmd_pass(Client &client, const Command &cmd);
	void	cmd_nick(Client &client, const Command &cmd);
	void	cmd_user(Client &client, const Command &cmd);
	void	cmd_join(Client &client, const Command &cmd);
	void	cmd_part(Client &client, const Command &cmd);
	void	cmd_privmsg(Client &client, const Command &cmd);
public:
	Server(int listen_fd, std::string password);
	~Server();
	void	loop();
};

#endif
