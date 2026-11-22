#include "../include/client.hpp"

Client::Client(int fd, std::string hostname)
	: fd(fd), hostname(hostname), password_given(false), registered(false)
{
}

// lit ce que le client a envoye, retourne 0 s'il s'est deconnecte
int	Client::read_data()
{
	char	buffer[512];
	int		ret = recv(this->fd, buffer, sizeof(buffer), 0);

	if (ret > 0)
		this->inbuffer.append(buffer, ret);
	// pas de \n apres 512 octets : on jette pour ne pas grossir a l'infini
	if (this->inbuffer.size() > 512 && this->inbuffer.find('\n') == std::string::npos)
		this->inbuffer.clear();
	return (ret);
}

// envoie ce qui est en attente, retourne 0 ou -1 en cas d'erreur
int	Client::write_data()
{
	int	ret = send(this->fd, this->outbuffer.c_str(), this->outbuffer.size(), 0);

	if (ret > 0)
		this->outbuffer.erase(0, ret);
	return (ret);
}

// recupere une ligne complete (finie par \n) de inbuffer, sans le \r\n
bool	Client::get_line(std::string &line)
{
	size_t	pos = this->inbuffer.find('\n');

	if (pos == std::string::npos)
		return (false);
	line = this->inbuffer.substr(0, pos);
	this->inbuffer.erase(0, pos + 1);
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);
	return (true);
}
