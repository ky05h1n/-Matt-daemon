#include "Md_header.hpp"


Atr::Atr()
{
    std::cout << "Starting Daemon..." << std::endl;
    logpath = "/var/log/";
    logfile = "matt-daemon.log";
    root = "/";
    Obj.fd = 0;
}

uid_t GetEffectiveUserId() {

    uid_t euid = geteuid();
    return euid;
}


Atr::~Atr()
{

}


bool Atr::CheckFiles_Dirs(){

        if (chdir(this->root.c_str()) == 0)
        {
            std::string fullpath = logpath + logfile;
            this->Obj.fd = open(fullpath.c_str(), O_RDWR | O_CREAT | O_APPEND, 0644);
            if (this->Obj.fd == -1)
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
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    if (pid == 0){
        pid_t s_id = setsid();
        if (s_id < 0)
        {
            std::cout << "Failed To Create New Session !" << std::endl;
            exit(1);
        }

        if (!this->CheckFiles_Dirs())
            exit(1);
        this->Run();
        exit(0);
        
    }
    else if (pid == -1){
    
        std::cout << "Fork Failed To Run !" << std::endl;
        return;
    }
    if (pid > 0)
        return;
}

