#include "../include/server.hpp"

volatile sig_atomic_t g_stop = 0;

// envoie ":ircserv <code> <nick> [args] :<text>" au client
static void	reply(Client &client, const std::string &code, const std::string &args, const std::string &text)
{
	std::string	nick = client.nickname;

	if (nick.empty())
		nick = "*";
	client.outbuffer += ":ircserv " + code + " " + nick + " ";
	if (!args.empty())
		client.outbuffer += args + " ";
	client.outbuffer += ":" + text + "\r\n";
}

// ":nick!user@host"
static std::string	prefix_of(const Client &client)
{
	return (":" + client.nickname + "!" + client.username + "@" + client.hostname);
}

Server::Server(int listen_fd, std::string password) : password(password), listen_fd(listen_fd)
{
	pollfd	p;

	p.fd = listen_fd;
	p.events = POLLIN;
	p.revents = 0;
	this->all_fd.push_back(p);
}

Server::~Server()
{
	for (size_t i = 0; i < this->all_fd.size(); i++)
		close(this->all_fd[i].fd);
}

void	Server::accept_client()
{
	sockaddr_in	addr;
	socklen_t	len = sizeof(addr);
	int			fd = accept(this->listen_fd, (sockaddr *)&addr, &len);
	pollfd		p;

	if (fd < 0)
		return ;
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(fd);
		return ;
	}
	p.fd = fd;
	p.events = POLLIN;
	p.revents = 0;
	this->all_fd.push_back(p);
	this->all_client.insert(std::make_pair(fd, Client(fd, inet_ntoa(addr.sin_addr))));
}

// retire fd du channel, et supprime le channel s'il est vide
// (sinon il garderait son topic et ses modes pour le prochain qui le cree)
void	Server::leave_channel(const std::string &name, int fd)
{
	Channel	&channel = this->all_channel[name];

	channel.members.erase(fd);
	channel.ops.erase(fd);
	channel.invited.erase(fd);
	if (channel.members.empty())
		this->all_channel.erase(name);
}

// ferme le client et le retire de ses channels
void	Server::remove_client(size_t i)
{
	int											fd = this->all_fd[i].fd;
	std::map<std::string, Channel>::iterator	it = this->all_channel.begin();

	while (it != this->all_channel.end())
	{
		// on avance avant, car leave_channel peut supprimer le channel
		std::string	name = it->first;
		it++;
		this->leave_channel(name, fd);
	}
	close(fd);
	this->all_client.erase(fd);
	this->all_fd.erase(this->all_fd.begin() + i);
}

// envoie msg a tous les membres du channel sauf except_fd
void	Server::send_channel(const std::string &name, const std::string &msg, int except_fd)
{
	std::set<int>			&members = this->all_channel[name].members;
	std::set<int>::iterator	it;

	for (it = members.begin(); it != members.end(); it++)
		if (*it != except_fd)
			this->all_client.find(*it)->second.outbuffer += msg;
}

// cherche un client enregistre par son pseudo, NULL s'il n'existe pas
Client	*Server::find_client(const std::string &nick)
{
	std::map<int, Client>::iterator	it;

	for (it = this->all_client.begin(); it != this->all_client.end(); it++)
		if (it->second.registered && it->second.nickname == nick)
			return (&it->second);
	return (NULL);
}

void	Server::cmd_pass(Client &client, const Command &cmd)
{
	if (cmd.params.empty())
	{
		reply(client, "461", "PASS", "Not enough parameters");
		return ;
	}
	if (cmd.params[0] == this->password)
		client.password_given = true;
	else
		reply(client, "464", "", "Password incorrect");
}

void	Server::cmd_nick(Client &client, const Command &cmd)
{
	if (cmd.params.empty())
	{
		reply(client, "431", "", "No nickname given");
		return ;
	}
	std::string	nick = cmd.params[0];
	std::map<int, Client>::iterator	it;
	for (it = this->all_client.begin(); it != this->all_client.end(); it++)
	{
		if (it->first != client.fd && it->second.nickname == nick)
		{
			reply(client, "433", nick, "Nickname is already in use");
			return ;
		}
	}
	client.nickname = nick;
}

void	Server::cmd_user(Client &client, const Command &cmd)
{
	if (cmd.params.empty())
	{
		reply(client, "461", "USER", "Not enough parameters");
		return ;
	}
	client.username = cmd.params[0];
}

void	Server::cmd_join(Client &client, const Command &cmd)
{
	if (cmd.params.empty())
	{
		reply(client, "461", "JOIN", "Not enough parameters");
		return ;
	}
	std::string	name = cmd.params[0];

	if (name[0] != '#')
	{
		reply(client, "403", name, "No such channel");
		return ;
	}
	Channel	&channel = this->all_channel[name];

	if (channel.members.empty())
		channel.ops.insert(client.fd);	// le premier arrive est operateur
	channel.members.insert(client.fd);
	this->send_channel(name, prefix_of(client) + " JOIN " + name + "\r\n", -1);
}

void	Server::cmd_privmsg(Client &client, const Command &cmd)
{
	if (cmd.params.empty())
	{
		reply(client, "411", "", "No recipient given (PRIVMSG)");
		return ;
	}
	if (cmd.params.size() < 2 || cmd.params[1].empty())
	{
		reply(client, "412", "", "No text to send");
		return ;
	}
	std::string	target = cmd.params[0];
	std::string	msg = prefix_of(client) + " PRIVMSG " + target + " :" + cmd.params[1] + "\r\n";

	if (target[0] == '#')
	{
		if (this->all_channel.count(target) == 0)
			reply(client, "403", target, "No such channel");
		else if (this->all_channel[target].members.count(client.fd) == 0)
			reply(client, "404", target, "Cannot send to channel");	// pas membre (ou kick)
		else
			this->send_channel(target, msg, client.fd);
		return ;
	}
	std::map<int, Client>::iterator	it;
	for (it = this->all_client.begin(); it != this->all_client.end(); it++)
	{
		if (it->second.registered && it->second.nickname == target)
		{
			it->second.outbuffer += msg;
			return ;
		}
	}
	reply(client, "401", target, "No such nick/channel");
}

// TOPIC #chan        -> affiche le topic
// TOPIC #chan :texte -> change le topic (operateur obligatoire si mode t)
void	Server::cmd_topic(Client &client, const Command &cmd)
{
	if (cmd.params.empty())
	{
		reply(client, "461", "TOPIC", "Not enough parameters");
		return ;
	}
	std::string	name = cmd.params[0];

	if (this->all_channel.count(name) == 0)
	{
		reply(client, "403", name, "No such channel");
		return ;
	}
	Channel	&channel = this->all_channel[name];

	if (channel.members.count(client.fd) == 0)
	{
		reply(client, "442", name, "You're not on that channel");
		return ;
	}
	if (cmd.params.size() == 1)
	{
		if (channel.topic.empty())
			reply(client, "331", name, "No topic is set");
		else
			reply(client, "332", name, channel.topic);
		return ;
	}
	if (channel.topic_restricted && channel.ops.count(client.fd) == 0)
	{
		reply(client, "482", name, "You're not channel operator");
		return ;
	}
	channel.topic = cmd.params[1];
	this->send_channel(name, prefix_of(client) + " TOPIC " + name + " :" + channel.topic + "\r\n", -1);
}

// KICK #chan pseudo [:raison] -> ejecte pseudo du channel (operateur obligatoire)
void	Server::cmd_kick(Client &client, const Command &cmd)
{
	if (cmd.params.size() < 2)
	{
		reply(client, "461", "KICK", "Not enough parameters");
		return ;
	}
	std::string	name = cmd.params[0];
	std::string	nick = cmd.params[1];

	if (this->all_channel.count(name) == 0)
	{
		reply(client, "403", name, "No such channel");
		return ;
	}
	Channel	&channel = this->all_channel[name];

	if (channel.members.count(client.fd) == 0)
	{
		reply(client, "442", name, "You're not on that channel");
		return ;
	}
	if (channel.ops.count(client.fd) == 0)
	{
		reply(client, "482", name, "You're not channel operator");
		return ;
	}
	Client	*target = this->find_client(nick);

	if (target == NULL)
	{
		reply(client, "401", nick, "No such nick/channel");
		return ;
	}
	if (channel.members.count(target->fd) == 0)
	{
		reply(client, "441", nick + " " + name, "They aren't on that channel");
		return ;
	}
	std::string	reason = client.nickname;

	if (cmd.params.size() > 2 && !cmd.params[2].empty())
		reason = cmd.params[2];
	// on previent tout le monde (la cible comprise) avant de la retirer
	this->send_channel(name, prefix_of(client) + " KICK " + name + " " + nick + " :" + reason + "\r\n", -1);
	this->leave_channel(name, target->fd);
}

// INVITE pseudo #chan -> autorise pseudo a rejoindre #chan (utile en mode i)
// si le channel est en mode i, seul un operateur peut inviter
void	Server::cmd_invite(Client &client, const Command &cmd)
{
	if (cmd.params.size() < 2)
	{
		reply(client, "461", "INVITE", "Not enough parameters");
		return ;
	}
	std::string	nick = cmd.params[0];
	std::string	name = cmd.params[1];
	Client		*target = this->find_client(nick);

	if (target == NULL)
	{
		reply(client, "401", nick, "No such nick/channel");
		return ;
	}
	if (this->all_channel.count(name) == 0)
	{
		reply(client, "403", name, "No such channel");
		return ;
	}
	Channel	&channel = this->all_channel[name];

	if (channel.members.count(client.fd) == 0)
	{
		reply(client, "442", name, "You're not on that channel");
		return ;
	}
	if (channel.invite_only && channel.ops.count(client.fd) == 0)
	{
		reply(client, "482", name, "You're not channel operator");
		return ;
	}
	if (channel.members.count(target->fd))
	{
		reply(client, "443", nick + " " + name, "is already on channel");
		return ;
	}
	channel.invited.insert(target->fd);
	// 341 confirme a celui qui invite, la cible recoit le INVITE
	client.outbuffer += ":ircserv 341 " + client.nickname + " " + nick + " " + name + "\r\n";
	target->outbuffer += prefix_of(client) + " INVITE " + nick + " " + name + "\r\n";
}

// PING :texte -> le client verifie que la connexion est vivante, on renvoie PONG
// (sans reponse, irssi/hexchat se deconnectent au bout de quelques minutes)
void	Server::cmd_ping(Client &client, const Command &cmd)
{
	if (cmd.params.empty())
	{
		reply(client, "409", "", "No origin specified");
		return ;
	}
	client.outbuffer += ":ircserv PONG ircserv :" + cmd.params[0] + "\r\n";
}

// quand PASS + NICK + USER sont faits, le client est enregistre
void	Server::try_register(Client &client)
{
	if (client.registered || !client.password_given)
		return ;
	if (client.nickname.empty() || client.username.empty())
		return ;
	client.registered = true;
	reply(client, "001", "", "Welcome to the IRC server " + prefix_of(client).substr(1));
}

void	Server::execute(Client &client, const std::string &line)
{
	Command	cmd(line);

	if (cmd.name.empty())
		return ;
	if (cmd.name == "PASS")
		this->cmd_pass(client, cmd);
	else if (cmd.name == "NICK")
		this->cmd_nick(client, cmd);
	else if (cmd.name == "USER")
		this->cmd_user(client, cmd);
	else if (cmd.name == "PING")
		this->cmd_ping(client, cmd);
	else if (!client.registered)
		reply(client, "451", "", "You have not registered");
	else if (cmd.name == "JOIN")
		this->cmd_join(client, cmd);
	else if (cmd.name == "PRIVMSG")
		this->cmd_privmsg(client, cmd);
	else if (cmd.name == "TOPIC")
		this->cmd_topic(client, cmd);
	else if (cmd.name == "KICK")
		this->cmd_kick(client, cmd);
	else if (cmd.name == "INVITE")
		this->cmd_invite(client, cmd);
	else
		reply(client, "421", cmd.name, "Unknown command");
	this->try_register(client);
}

// gere ce que poll() a signale pour le client i, retourne true si on l'a supprime
bool	Server::handle_client(size_t i)
{
	short	revents = this->all_fd[i].revents;
	Client	&client = this->all_client.find(this->all_fd[i].fd)->second;
	std::string	line;

	if (revents & POLLIN)
	{
		if (client.read_data() == 0)
		{
			this->remove_client(i);
			return (true);
		}
		while (client.get_line(line))
			this->execute(client, line);
	}
	if ((revents & POLLOUT) && client.write_data() <= 0)
	{
		this->remove_client(i);
		return (true);
	}
	if (revents & (POLLHUP | POLLERR | POLLNVAL))
	{
		this->remove_client(i);
		return (true);
	}
	return (false);
}

void	Server::loop()
{
	while (!g_stop)
	{
		// on ecoute POLLOUT seulement si on a quelque chose a envoyer
		for (size_t i = 1; i < this->all_fd.size(); i++)
		{
			this->all_fd[i].events = POLLIN;
			if (!this->all_client.find(this->all_fd[i].fd)->second.outbuffer.empty())
				this->all_fd[i].events = POLLIN | POLLOUT;
		}
		if (poll(&this->all_fd[0], this->all_fd.size(), -1) < 0)
			continue ;
		if (this->all_fd[0].revents & POLLIN)
			this->accept_client();
		for (size_t i = 1; i < this->all_fd.size(); i++)
			if (this->handle_client(i))
				i--;
	}
}
