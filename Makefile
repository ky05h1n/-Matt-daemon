

CC = g++

CFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS = matt-daemon.cpp

all: Matt_daemon

Matt_daemon: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $@

clean:
	rm -f Matt_daemon

re: clean all