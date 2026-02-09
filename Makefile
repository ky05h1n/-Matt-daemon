CC = g++

CFLAGS = -Wall -Wextra -Werror

SRCS = src/MattDaemon.cpp src/Build_damon.cpp src/Logs.cpp src/Socket.cpp

NAME = Matt_daemon

all: $(NAME)

$(NAME): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(NAME)

clean:
	rm -f $(NAME)

fclean: clean

re: fclean all

.PHONY: all clean fclean re