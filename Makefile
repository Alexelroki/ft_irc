NAME = ircserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude
DEPFLAGS = -MMD -MP

SRC_DIR = src
OBJ_DIR = .objs

SRCS =	main.cpp \
		Server.cpp \
		Client.cpp \
		Parser.cpp \
		CommandHandler.cpp \
		Channel.cpp \
		cmdChannel.cpp \
		utilsChannel.cpp \
		Replies.cpp

OBJS = $(addprefix $(OBJ_DIR)/,$(SRCS:.cpp=.o))
DEPS = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re
