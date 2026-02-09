
#include "../header/Md_header.hpp"

Tintin_reporter::Tintin_reporter()
{

}
 Tintin_reporter::~Tintin_reporter()
{

}

void Tintin_reporter::Log(std::string logmessage, LogLevel level) {
    if (this->fd < 0)
        return;  
    
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "[%d/%m/%Y-%H:%M:%S]", timeinfo);
    
    std::string levelStr;
    switch (level) {
        case INFO:
            levelStr = "[ INFO ]";
            break;
        case LOG:
            levelStr = "[ LOG ]";
            break;
        case ERROR:
            levelStr = "[ ERROR ]";
            break;
        default:
            levelStr = "[ INFO ]";
    }
    
    std::string logEntry = std::string(timestamp) + " " + levelStr + " - " + logmessage + "\n";
    write(this->fd, logEntry.c_str(), logEntry.length());
}