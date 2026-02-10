#pragma once

#include "Md_header.hpp"
#include <string>
#include <vector>

struct SmtpConfig {
    bool enabled;
    std::string host;
    int port;
    std::string helo;
    std::string user;
    std::string pass;
    std::string from;
    std::vector<std::string> to;
    std::string subject;
    std::string tlsMode;
    LogLevel minLevel;
    std::vector<std::string> keywords;
};

bool load_smtp_config(SmtpConfig &cfg);
bool should_send_mail(const SmtpConfig &cfg, LogLevel level, const std::string &message);
bool send_smtp_mail(const SmtpConfig &cfg, const std::string &body);
