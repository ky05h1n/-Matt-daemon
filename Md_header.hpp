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
    INFO,
    LOG,
    ERROR
};
class Tintin_reporter{

    public :
            int fd;
            Tintin_reporter();
            ~Tintin_reporter();
            void Log(std::string logmessage, LogLevel level = INFO);
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
