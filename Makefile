NAME     := webserv
CXX      := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -MMD -MP
SRCDIR   := srcs
OBJDIR   := objs
SRCS     := $(SRCDIR)/main.cpp $(SRCDIR)/server/Server.cpp $(SRCDIR)/Utils.cpp \
            $(SRCDIR)/config/ConfigParse.cpp \
			$(SRCDIR)/event/Multiplexer.cpp \
			$(SRCDIR)/event/SelectMultiplexer.cpp \
			$(SRCDIR)/event/PollMultiplexer.cpp \
			$(SRCDIR)/http/HttpRequest.cpp \
			$(SRCDIR)/http/HttpResponse.cpp \
			$(SRCDIR)/client/Client.cpp \
			$(SRCDIR)/http/HttpRequestParser.cpp

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  SRCS += srcs/event/KqueueMultiplexer.cpp
endif

ifeq ($(UNAME_S),Linux)
  SRCS += srcs/event/EpollMultiplexer.cpp
endif

OBJS     := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))
DEPS     := $(OBJS:.o=.d)

RM = rm -rf
INCLUDES := -I./includes

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(NAME) $(OBJS)

-include $(DEPS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR):
	mkdir -p $@

clean:
	$(RM) $(OBJDIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

run: $(NAME)
	@echo "== Running Tests  =="
	./$(NAME) config/valid/multiple_servers.conf
	@echo "== Tests Completed =="

test:
	@echo "== Running Tests  =="
	siege -c 10 -r 5 --time=10S --log=/tmp/siege.log http://localhost:8080
	@echo "== Tests Completed =="

.PHONY: all clean fclean re run test
