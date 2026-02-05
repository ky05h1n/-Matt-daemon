#include "matt-daemon.hpp"


Atr::Atr()
{
    logpath = "/var/log/";
    logfile = "matt-daemon.log";
    fd = 0;
    root = "/";
}

uid_t GetEffectiveUserId() {

    uid_t euid = geteuid();
    std::cout << "e_uid : "<< euid << std::endl;
    return euid;
}


void Atr::Log(std::string logmessage)
{
    
}

Atr::~Atr()
{
    std::cout << "Daemon Stopped !" << std::endl;
}


bool Atr::CheckFiles_Dirs(){

        if (chdir(this->root.c_str()) == 0)
        {
            std::string fullpath = logpath + logfile;
            this->fd = open(fullpath.c_str(), O_RDWR | O_CREAT | O_APPEND, 0644);
            if (fd == -1)
            {
                std::cout << "daemon : can't create log file" << std::endl;
                return false;
            }
            return true;
        }
        else
        {
            std::cout << "daemon : can't access to 'root' folder" << std::endl;
            return false;
        }
}



void Atr::Daemon(){

    
    pid_t pid = fork();
    if (pid == 0){
        pid_t s_id = setsid();
        if (s_id < 0)
        {
            std::cout << "Failed To Create New Session !" << std::endl;
            exit(0);
        }
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        if (!this->CheckFiles_Dirs())
            exit(0);
        
    }
    else if (pid == -1){
    
        std::cout << "Fork Failed To Run !" << std::endl;
        exit(0);
    }
    if (pid > 0)
        exit(0);
}

