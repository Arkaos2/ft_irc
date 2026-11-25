#ifndef CHANNEL
# define CHANNEL
# include "include.hpp"

// un channel garde les fd de ses membres, de ses operateurs, et ses modes
class Channel
{
public:
	std::set<int>	members;
	std::set<int>	ops;
	std::set<int>	invited;			// fd des clients invites (mode i)
	std::string		topic;
	std::string		key;				// mode k (vide = pas de mot de passe)
	size_t			limit;				// mode l (0 = pas de limite)
	bool			invite_only;		// mode i
	bool			topic_restricted;	// mode t

	Channel() : limit(0), invite_only(false), topic_restricted(false) {}
};

#endif
