#include "Mail.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

static std::string get_env_value(const char *key) {
    const char *value = std::getenv(key);
    return value ? std::string(value) : std::string();
}

static std::string trim(const std::string &value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        start++;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        end--;
    }
    return value.substr(start, end - start);
}

static std::string to_upper(const std::string &value) {
    std::string out = value;
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[i])));
    }
    return out;
}

static std::string to_lower(const std::string &value) {
    std::string out = value;
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    }
    return out;
}

static std::vector<std::string> split_csv(const std::string &value) {
    std::vector<std::string> parts;
    std::string token;
    std::stringstream ss(value);
    while (std::getline(ss, token, ',')) {
        std::string trimmed = trim(token);
        if (!trimmed.empty()) {
            parts.push_back(trimmed);
        }
    }
    return parts;
}

static LogLevel parse_level(const std::string &value) {
    std::string upper = to_upper(trim(value));
    if (upper == "INFO") {
        return LEVEL_INFO;
    }
    if (upper == "LOG") {
        return LEVEL_LOG;
    }
    if (upper == "ERROR") {
        return LEVEL_ERROR;
    }
    return LEVEL_ERROR;
}

bool load_smtp_config(SmtpConfig &cfg) {
    cfg.enabled = false;
    cfg.host = trim(get_env_value("MD_SMTP_HOST"));
    cfg.port = 25;
    std::string port_str = trim(get_env_value("MD_SMTP_PORT"));
    if (!port_str.empty()) {
        cfg.port = std::atoi(port_str.c_str());
    }
    cfg.helo = trim(get_env_value("MD_SMTP_HELO"));
    if (cfg.helo.empty()) {
        cfg.helo = "localhost";
    }
    cfg.user = trim(get_env_value("MD_SMTP_USER"));
    cfg.pass = trim(get_env_value("MD_SMTP_PASS"));
    if (cfg.user.empty() || cfg.pass.empty()) {
        cfg.user.clear();
        cfg.pass.clear();
    }
    cfg.from = trim(get_env_value("MD_SMTP_FROM"));
    if (cfg.from.empty()) {
        cfg.from = "matt-daemon@localhost";
    }
    cfg.subject = trim(get_env_value("MD_SMTP_SUBJECT"));
    if (cfg.subject.empty()) {
        cfg.subject = "matt-daemon alert";
    }
    cfg.tlsMode = to_lower(trim(get_env_value("MD_SMTP_TLS")));
    cfg.minLevel = parse_level(get_env_value("MD_SMTP_MIN_LEVEL"));
    cfg.keywords = split_csv(get_env_value("MD_SMTP_KEYWORDS"));
    cfg.to = split_csv(get_env_value("MD_SMTP_TO"));

    if (cfg.host.empty() || cfg.to.empty()) {
        return false;
    }
    cfg.enabled = true;
    return true;
}

static bool icontains(const std::string &text, const std::string &needle) {
    if (needle.empty()) {
        return true;
    }
    std::string lower_text = to_lower(text);
    std::string lower_needle = to_lower(needle);
    return lower_text.find(lower_needle) != std::string::npos;
}

bool should_send_mail(const SmtpConfig &cfg, LogLevel level, const std::string &message) {
    if (!cfg.enabled) {
        return false;
    }
    if (level < cfg.minLevel) {
        return false;
    }
    if (cfg.keywords.empty()) {
        return true;
    }
    for (size_t i = 0; i < cfg.keywords.size(); ++i) {
        if (icontains(message, cfg.keywords[i])) {
            return true;
        }
    }
    return false;
}

static bool send_all(int fd, const std::string &data) {
    size_t total = 0;
    while (total < data.size()) {
        ssize_t sent = send(fd, data.data() + total, data.size() - total, 0);
        if (sent <= 0) {
            if (sent < 0 && errno == EINTR) {
                continue;
            }
            return false;
        }
        total += static_cast<size_t>(sent);
    }
    return true;
}

static bool send_all_ssl(SSL *ssl, const std::string &data) {
    size_t total = 0;
    while (total < data.size()) {
        int sent = SSL_write(ssl, data.data() + total, static_cast<int>(data.size() - total));
        if (sent <= 0) {
            return false;
        }
        total += static_cast<size_t>(sent);
    }
    return true;
}

static bool read_line(int fd, std::string &out) {
    out.clear();
    char ch = 0;
    while (true) {
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n <= 0) {
            return false;
        }
        out.push_back(ch);
        if (ch == '\n') {
            return true;
        }
        if (out.size() > 4096) {
            return false;
        }
    }
}

static bool read_line_ssl(SSL *ssl, std::string &out) {
    out.clear();
    char ch = 0;
    while (true) {
        int n = SSL_read(ssl, &ch, 1);
        if (n <= 0) {
            return false;
        }
        out.push_back(ch);
        if (ch == '\n') {
            return true;
        }
        if (out.size() > 4096) {
            return false;
        }
    }
}

static bool smtp_read_response(int fd, int &code) {
    std::string line;
    code = 0;
    while (true) {
        if (!read_line(fd, line)) {
            return false;
        }
        if (line.size() < 4) {
            continue;
        }
        code = std::atoi(line.substr(0, 3).c_str());
        if (line[3] == ' ') {
            return true;
        }
    }
}

static bool smtp_read_response_ssl(SSL *ssl, int &code) {
    std::string line;
    code = 0;
    while (true) {
        if (!read_line_ssl(ssl, line)) {
            return false;
        }
        if (line.size() < 4) {
            continue;
        }
        code = std::atoi(line.substr(0, 3).c_str());
        if (line[3] == ' ') {
            return true;
        }
    }
}

static bool smtp_expect(int fd, int expected) {
    int code = 0;
    if (!smtp_read_response(fd, code)) {
        return false;
    }
    return code == expected;
}

static bool smtp_expect_ssl(SSL *ssl, int expected) {
    int code = 0;
    if (!smtp_read_response_ssl(ssl, code)) {
        return false;
    }
    return code == expected;
}

static bool smtp_send_line(int fd, const std::string &line) {
    return send_all(fd, line + "\r\n");
}

static bool smtp_send_line_ssl(SSL *ssl, const std::string &line) {
    return send_all_ssl(ssl, line + "\r\n");
}

static std::string base64_encode(const std::string &input) {
    static const char *table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    size_t i = 0;
    for (; i + 2 < input.size(); i += 3) {
        unsigned int triple = (static_cast<unsigned char>(input[i]) << 16)
                            | (static_cast<unsigned char>(input[i + 1]) << 8)
                            | static_cast<unsigned char>(input[i + 2]);
        out.push_back(table[(triple >> 18) & 0x3F]);
        out.push_back(table[(triple >> 12) & 0x3F]);
        out.push_back(table[(triple >> 6) & 0x3F]);
        out.push_back(table[triple & 0x3F]);
    }

    if (i < input.size()) {
        unsigned int triple = static_cast<unsigned char>(input[i]) << 16;
        out.push_back(table[(triple >> 18) & 0x3F]);
        if (i + 1 < input.size()) {
            triple |= static_cast<unsigned char>(input[i + 1]) << 8;
            out.push_back(table[(triple >> 12) & 0x3F]);
            out.push_back(table[(triple >> 6) & 0x3F]);
            out.push_back('=');
        } else {
            out.push_back(table[(triple >> 12) & 0x3F]);
            out.push_back('=');
            out.push_back('=');
        }
    }
    return out;
}

static int connect_smtp(const std::string &host, int port) {
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::stringstream port_stream;
    port_stream << port;

    if (getaddrinfo(host.c_str(), port_stream.str().c_str(), &hints, &result) != 0) {
        return -1;
    }

    int fd = -1;
    for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            continue;
        }
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }

    freeaddrinfo(result);
    return fd;
}

static SSL_CTX *create_ssl_ctx() {
    SSL_library_init();
    SSL_load_error_strings();
    const SSL_METHOD *method = TLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        return NULL;
    }
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    return ctx;
}

static SSL *wrap_ssl(SSL_CTX *ctx, int fd) {
    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        return NULL;
    }
    SSL_set_fd(ssl, fd);
    if (SSL_connect(ssl) != 1) {
        SSL_free(ssl);
        return NULL;
    }
    return ssl;
}

static std::string build_data(const SmtpConfig &cfg, const std::string &body) {
    std::stringstream out;
    out << "Subject: " << cfg.subject << "\r\n";
    out << "From: " << cfg.from << "\r\n";
    out << "To: ";
    for (size_t i = 0; i < cfg.to.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << cfg.to[i];
    }
    out << "\r\n";
    out << "Content-Type: text/plain; charset=UTF-8\r\n";
    out << "\r\n";

    std::stringstream body_stream(body);
    std::string line;
    while (std::getline(body_stream, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }
        if (!line.empty() && line[0] == '.') {
            out << '.';
        }
        out << line << "\r\n";
    }
    return out.str();
}

bool send_smtp_mail(const SmtpConfig &cfg, const std::string &body) {
    if (!cfg.enabled) {
        return false;
    }

    int fd = connect_smtp(cfg.host, cfg.port);
    if (fd < 0) {
        return false;
    }

    SSL_CTX *ssl_ctx = NULL;
    SSL *ssl = NULL;
    bool use_ssl = (cfg.tlsMode == "ssl" || cfg.tlsMode == "smtps");
    bool use_starttls = (cfg.tlsMode == "starttls");
    if (use_ssl) {
        ssl_ctx = create_ssl_ctx();
        if (!ssl_ctx) {
            close(fd);
            return false;
        }
        ssl = wrap_ssl(ssl_ctx, fd);
        if (!ssl) {
            SSL_CTX_free(ssl_ctx);
            close(fd);
            return false;
        }
    }

    bool ok = use_ssl ? smtp_expect_ssl(ssl, 220) : smtp_expect(fd, 220);
    if (ok) {
        ok = use_ssl ? (smtp_send_line_ssl(ssl, "EHLO " + cfg.helo) && smtp_expect_ssl(ssl, 250))
                     : (smtp_send_line(fd, "EHLO " + cfg.helo) && smtp_expect(fd, 250));
    }
    if (!ok) {
        ok = use_ssl ? (smtp_send_line_ssl(ssl, "HELO " + cfg.helo) && smtp_expect_ssl(ssl, 250))
                     : (smtp_send_line(fd, "HELO " + cfg.helo) && smtp_expect(fd, 250));
    }

    if (ok && use_starttls) {
        ok = smtp_send_line(fd, "STARTTLS") && smtp_expect(fd, 220);
        if (ok) {
            ssl_ctx = create_ssl_ctx();
            if (!ssl_ctx) {
                close(fd);
                return false;
            }
            ssl = wrap_ssl(ssl_ctx, fd);
            if (!ssl) {
                SSL_CTX_free(ssl_ctx);
                close(fd);
                return false;
            }
            ok = smtp_send_line_ssl(ssl, "EHLO " + cfg.helo) && smtp_expect_ssl(ssl, 250);
        }
    }

    if (ok && !cfg.user.empty()) {
        if (ssl) {
            ok = smtp_send_line_ssl(ssl, "AUTH LOGIN") && smtp_expect_ssl(ssl, 334);
            if (ok) {
                ok = smtp_send_line_ssl(ssl, base64_encode(cfg.user)) && smtp_expect_ssl(ssl, 334);
            }
            if (ok) {
                ok = smtp_send_line_ssl(ssl, base64_encode(cfg.pass)) && smtp_expect_ssl(ssl, 235);
            }
        } else {
            ok = smtp_send_line(fd, "AUTH LOGIN") && smtp_expect(fd, 334);
            if (ok) {
                ok = smtp_send_line(fd, base64_encode(cfg.user)) && smtp_expect(fd, 334);
            }
            if (ok) {
                ok = smtp_send_line(fd, base64_encode(cfg.pass)) && smtp_expect(fd, 235);
            }
        }
    }

    if (ok) {
        if (ssl) {
            ok = smtp_send_line_ssl(ssl, "MAIL FROM:<" + cfg.from + ">") && smtp_expect_ssl(ssl, 250);
        } else {
            ok = smtp_send_line(fd, "MAIL FROM:<" + cfg.from + ">") && smtp_expect(fd, 250);
        }
    }

    for (size_t i = 0; ok && i < cfg.to.size(); ++i) {
        if (ssl) {
            ok = smtp_send_line_ssl(ssl, "RCPT TO:<" + cfg.to[i] + ">") && smtp_expect_ssl(ssl, 250);
        } else {
            ok = smtp_send_line(fd, "RCPT TO:<" + cfg.to[i] + ">") && smtp_expect(fd, 250);
        }
    }

    if (ok) {
        if (ssl) {
            ok = smtp_send_line_ssl(ssl, "DATA") && smtp_expect_ssl(ssl, 354);
        } else {
            ok = smtp_send_line(fd, "DATA") && smtp_expect(fd, 354);
        }
    }

    if (ok) {
        std::string data = build_data(cfg, body);
        if (ssl) {
            ok = send_all_ssl(ssl, data) && send_all_ssl(ssl, "\r\n.\r\n") && smtp_expect_ssl(ssl, 250);
        } else {
            ok = send_all(fd, data) && send_all(fd, "\r\n.\r\n") && smtp_expect(fd, 250);
        }
    }

    if (ssl) {
        smtp_send_line_ssl(ssl, "QUIT");
        SSL_shutdown(ssl);
        SSL_free(ssl);
    } else {
        smtp_send_line(fd, "QUIT");
    }
    if (ssl_ctx) {
        SSL_CTX_free(ssl_ctx);
    }
    close(fd);
    return ok;
}
