
#include "../header/Md_header.hpp"
#include "../bonus/Mail.hpp"

Tintin_reporter::Tintin_reporter()
{
    this->fd = -1;
    this->currentLevel = LEVEL_INFO;
    this->archivePath = "/var/log/archives/";
}
 Tintin_reporter::~Tintin_reporter()
{

}

void Tintin_reporter::setLogFilePath(const std::string &path) {
    this->logFilePath = path;
}

void Tintin_reporter::setLogLevel(LogLevel level) {
    this->currentLevel = level;
    this->Log("Log level changed", LEVEL_INFO);
}

LogLevel Tintin_reporter::getLogLevel() const {
    return this->currentLevel;
}

size_t Tintin_reporter::getLogFileSize() const {
    if (this->logFilePath.empty()) {
        return 0;
    }
    struct stat st;
    if (stat(this->logFilePath.c_str(), &st) != 0) {
        return 0;
    }
    return static_cast<size_t>(st.st_size);
}

void Tintin_reporter::checkAndArchive() {
    if (this->logFilePath.empty()) {
        return;
    }
    struct stat st;
    if (stat(this->logFilePath.c_str(), &st) != 0) {
        return;
    }
    if (static_cast<size_t>(st.st_size) <= MAX_LOG_SIZE) {
        return;
    }

    mkdir(this->archivePath.c_str(), 0755);

    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    char ts[64];
    strftime(ts, sizeof(ts), "matt_daemon_%Y-%m-%d_%H-%M-%S.log", timeinfo);
    std::string archived = this->archivePath + ts;

    if (this->fd >= 0) {
        close(this->fd);
        this->fd = -1;
    }

    std::rename(this->logFilePath.c_str(), archived.c_str());

    this->fd = open(this->logFilePath.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (this->fd >= 0) {
        std::string header = "[INFO] Previous log archived.\n";
        write(this->fd, header.c_str(), header.length());
    }
}

void Tintin_reporter::Log(std::string logmessage, LogLevel level) {
    if (this->fd < 0)
        return;
    if (level < this->currentLevel)
        return;

    checkAndArchive();
    
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "[%d/%m/%Y-%H:%M:%S]", timeinfo);
    
    std::string levelStr;
    switch (level) {
        case LEVEL_INFO:
            levelStr = "[ INFO ]";
            break;
        case LEVEL_LOG:
            levelStr = "[ LOG ]";
            break;
        case LEVEL_ERROR:
            levelStr = "[ ERROR ]";
            break;
        default:
            levelStr = "[ INFO ]";
    }
    
    std::string logEntry = std::string(timestamp) + " " + levelStr + " - " + logmessage + "\n";
    write(this->fd, logEntry.c_str(), logEntry.length());

    static bool smtpLoaded = false;
    static SmtpConfig smtpConfig;
    if (!smtpLoaded) {
        load_smtp_config(smtpConfig);
        smtpLoaded = true;
    }
    if (should_send_mail(smtpConfig, level, logmessage)) {
        send_smtp_mail(smtpConfig, logEntry);
    }
}