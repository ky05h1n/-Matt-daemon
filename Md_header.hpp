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

    private :
            std::string logpath;
            std::string logfile;
            std::string root;
            Tintin_reporter Obj;

    public :
            Atr();
            ~Atr();
            void Daemon(void);
            bool CheckFiles_Dirs();
            void Run();

};



uid_t GetEffectiveUserId();
