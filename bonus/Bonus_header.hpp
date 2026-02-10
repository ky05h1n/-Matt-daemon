#pragma once

#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <sys/select.h>
#include <sstream>
#include <ctime>
#include <string>
#include <sys/socket.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

bool read_exact(int fd, unsigned char *buf, size_t len);
bool write_all(int fd, const unsigned char *buf, size_t len);

std::string rot13_transform(const std::string &text);

bool send_message(int fd, bool use_cipher, const std::string &plaintext);
bool recv_message(int fd, bool use_cipher, std::string &out);

bool append_log_line(const std::string &line);
