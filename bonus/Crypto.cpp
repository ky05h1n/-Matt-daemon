#include "Bonus_header.hpp"

bool read_exact(int fd, unsigned char *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t r = recv(fd, buf + total, len - total, 0);
        if (r == 0) {
            return false;
        }
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        total += static_cast<size_t>(r);
    }
    return true;
}

bool write_all(int fd, const unsigned char *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t w = send(fd, buf + total, len - total, 0);
        if (w <= 0) {
            if (w < 0 && errno == EINTR) {
                continue;
            }
            return false;
        }
        total += static_cast<size_t>(w);
    }
    return true;
}

static bool write_file_all(int fd, const char *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t w = write(fd, buf + total, len - total);
        if (w <= 0) {
            if (w < 0 && errno == EINTR) {
                continue;
            }
            return false;
        }
        total += static_cast<size_t>(w);
    }
    return true;
}

std::string rot13_transform(const std::string &text) {
    std::string out = text;
    for (size_t i = 0; i < out.size(); ++i) {
        char c = out[i];
        if (c >= 'a' && c <= 'z') {
            out[i] = static_cast<char>('a' + (c - 'a' + 13) % 26);
        } else if (c >= 'A' && c <= 'Z') {
            out[i] = static_cast<char>('A' + (c - 'A' + 13) % 26);
        }
    }
    return out;
}

bool send_message(int fd, bool use_cipher, const std::string &plaintext) {
    std::string payload = use_cipher ? rot13_transform(plaintext) : plaintext;
    uint32_t len = static_cast<uint32_t>(payload.size());
    uint32_t len_net = htonl(len);
    if (!write_all(fd, reinterpret_cast<unsigned char *>(&len_net), sizeof(len_net))) {
        return false;
    }
    if (len > 0) {
        if (!write_all(fd, reinterpret_cast<const unsigned char *>(payload.data()), payload.size())) {
            return false;
        }
    }
    return true;
}

bool recv_message(int fd, bool use_cipher, std::string &out) {
    uint32_t len_net = 0;
    if (!read_exact(fd, reinterpret_cast<unsigned char *>(&len_net), sizeof(len_net))) {
        return false;
    }
    uint32_t len = ntohl(len_net);
    std::string payload;
    payload.assign(len, '\0');
    if (len > 0) {
        if (!read_exact(fd, reinterpret_cast<unsigned char *>(&payload[0]), len)) {
            return false;
        }
    }
    out = use_cipher ? rot13_transform(payload) : payload;
    return true;
}

static bool archive_bonus_log_if_needed() {
    const char *archive_dir = "/var/log/matt_daemon/archive/";
    const char *path = "/var/log/matt_daemon.log";
    const size_t max_size = 1000;

    struct stat st;
    if (stat(path, &st) != 0) {
        return true;
    }
    if (static_cast<size_t>(st.st_size) <= max_size) {
        return true;
    }

    mkdir("/var/log/matt_daemon", 0755);
    mkdir(archive_dir, 0755);

    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char ts[64];
    strftime(ts, sizeof(ts), "matt_daemon_%Y-%m-%d_%H-%M-%S.log", timeinfo);
    std::string archived = std::string(archive_dir) + ts;
    std::rename(path, archived.c_str());
    return true;
}

bool append_log_line(const std::string &line) {
    const char *path = "/var/log/matt_daemon.log";
    archive_bonus_log_if_needed();
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        return false;
    }
    if (flock(fd, LOCK_EX) != 0) {
        close(fd);
        return false;
    }
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "[%d/%m/%Y-%H:%M:%S]", timeinfo);

    std::string msg = line;
    if (msg.rfind("[BONUS]", 0) == 0) {
        msg = msg.substr(7);
        if (!msg.empty() && msg[0] == ' ') {
            msg.erase(0, 1);
        }
    }

    std::string out = std::string(timestamp) + " [BONUS] [ LOG ] - " + msg;
    if (out.empty() || out[out.size() - 1] != '\n') {
        out += "\n";
    }
    bool ok = write_file_all(fd, out.c_str(), out.size());
    flock(fd, LOCK_UN);
    close(fd);
    return ok;
}
