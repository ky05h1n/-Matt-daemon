#pragma once

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <cstring>      
#include <sstream>  
#include <ctime>
#include <sys/file.h>
#include <sys/stat.h>
#include <csignal>
#include <cerrno>

enum LogLevel {
    LEVEL_INFO = 0,
    LEVEL_LOG = 1,
    LEVEL_ERROR = 2
};
class Tintin_reporter{

    public :
            int fd;
            LogLevel currentLevel;
            std::string logFilePath;
            std::string archivePath;
            // I think the recommended size is 1MB.
            static const size_t MAX_LOG_SIZE = 500;
            Tintin_reporter();
            ~Tintin_reporter();
            void Log(std::string logmessage, LogLevel level = LEVEL_INFO);
            void setLogLevel(LogLevel level);
            LogLevel getLogLevel() const;
            void setLogFilePath(const std::string &path);
            size_t getLogFileSize() const;
    private:
            void checkAndArchive();
};

class Atr{

    public :
            std::string logpath;
            std::string logfile;
            std::string root;
            std::string lockFilePath;
            int lockFd;
            Tintin_reporter Obj;
            int tempfd;
    
            
            Atr();
            ~Atr();
            void Daemon(void);
            bool CheckFiles_Dirs();
            void Run();
            bool CreateLockFile();
            void RemoveLockfile();

};



uid_t GetEffectiveUserId();
