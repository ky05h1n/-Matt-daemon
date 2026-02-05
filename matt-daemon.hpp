#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <string>


class Atr{

    private :

            std::string logpath;
            std::string logfile;
            std::string root;
            int fd;
    public :
            Atr::Atr();
            Atr::~Atr();
            void Daemon(void);
            void Log(std::string logmessage);
            bool CheckFiles_Dirs();     

}Atrr;



uid_t GetEffectiveUserId();
