NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
SRCDIR = srcs
SRCS = $(SRCDIR)/main.cpp $(SRCDIR)/server/server.cpp $(SRCDIR)/utils.cpp $(SRCDIR)/config/config_parse.cpp
OBJS = $(SRCS:.cpp=.o)

RM = rm -rf
INCLUDES = -I./includes

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(NAME) $(OBJS)

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re