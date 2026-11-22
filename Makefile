NAME	= ircserv
CXX		= c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRC		= main.cpp src/server.cpp src/client.cpp src/command.cpp
OBJ		= $(SRC:.cpp=.o)
HEADERS	= include/include.hpp include/server.hpp include/client.hpp include/command.hpp include/channel.hpp

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
