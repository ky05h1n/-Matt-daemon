CC = g++

CFLAGS = -Wall -Wextra -Werror
CPPFLAGS = -I. -Iheader -Ibonus

SRCS = src/MattDaemon.cpp src/Build_damon.cpp src/Logs.cpp src/Socket.cpp src/ServerCore.cpp bonus/Mail.cpp

BONUS_SRCS_DAEMON = bonus/BonusDaemon.cpp bonus/Crypto.cpp bonus/Mail.cpp src/ServerCore.cpp
BONUS_SRCS_CLIENT = bonus/BonusClient.cpp bonus/Crypto.cpp

NAME = Matt_daemon

BONUS_DAEMON = Matt_daemon_bonus
BONUS_CLIENT = Ben_AFK

MAIL_TEST = mail_filter_test
MAIL_TEST_SRCS = tools/mail_filter_test.cpp bonus/Mail.cpp


all: $(NAME)

$(NAME): $(SRCS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SRCS) -lssl -lcrypto -o $(NAME)

bonus: $(BONUS_DAEMON) $(BONUS_CLIENT)
mail_test: $(MAIL_TEST_SRCS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(MAIL_TEST_SRCS) -lssl -lcrypto -o $(MAIL_TEST)


hex_key:
	openssl rand -hex 32 > hex_key

export_data:
	@set -a; . ./.env; set +a; \
	  (echo "#!/usr/bin/env bash"; \
	   env -0 | awk -v RS='\0' -F= '/^MD_/ {val=substr($$0,index($$0,"=")+1); gsub(/"/, "\\\"", val); printf "export %s=\"%s\"\n", $$1, val}') > export_data
rm_data_file:
	rm export_data

$(BONUS_DAEMON): $(BONUS_SRCS_DAEMON)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(BONUS_SRCS_DAEMON) -lssl -lcrypto -o $(BONUS_DAEMON)

$(BONUS_CLIENT): $(BONUS_SRCS_CLIENT)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(BONUS_SRCS_CLIENT) -o $(BONUS_CLIENT)

clean:
	rm -f $(NAME) $(BONUS_DAEMON) $(BONUS_CLIENT) $(MAIL_TEST) hex_key

fclean: clean

re: fclean all

.PHONY: all bonus hex_key mail_test env export_data clean fclean re