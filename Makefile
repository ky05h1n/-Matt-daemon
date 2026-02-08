CC = g++

CFLAGS = -Wall -Wextra -Werror

SRCS = MattDaemon.cpp Build_damon.cpp Logs.cpp Socket.cpp

NAME = Matt_daemon

all: $(NAME)

$(NAME): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(NAME)

clean:
	rm -f $(NAME)

fclean: clean

re: fclean all

.PHONY: all clean fclean re