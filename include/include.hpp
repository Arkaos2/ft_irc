#ifndef INCLUDE_HPP
# define INCLUDE_HPP

# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <poll.h>
# include <fcntl.h>
# include <unistd.h>
# include <csignal>
# include <cstdio>
# include <cstring>
# include <cstdlib>
# include <iostream>
# include <string>
# include <sstream>
# include <vector>
# include <map>
# include <set>

// passe a 1 par ctrl-c pour arreter le serveur proprement
extern volatile sig_atomic_t g_stop;

#endif
