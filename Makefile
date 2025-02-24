NAME     := webserv
CXX      := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -MMD -MP
SRCDIR   := srcs
OBJDIR   := objs
SRCS     := $(SRCDIR)/main.cpp $(SRCDIR)/server/Server.cpp $(SRCDIR)/utils.cpp \
            $(SRCDIR)/config/config_parse.cpp \
            $(SRCDIR)/event/EpollMultiplexer.cpp \
			$(SRCDIR)/event/Multiplexer.cpp \
			$(SRCDIR)/event/SelectMultiplexer.cpp \
			$(SRCDIR)/event/KqueueMultiplexer.cpp \
			$(SRCDIR)/event/PollMultiplexer.cpp
OBJS     := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))
DEPS     := $(OBJS:.o=.d)

RM = rm -rf
INCLUDES := -I./includes

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(NAME) $(OBJS)

-include $(DEPS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
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

.PHONY: all clean fclean re run
