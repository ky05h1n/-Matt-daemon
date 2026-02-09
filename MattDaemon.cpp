#include "Md_header.hpp"


int main(){

    if (GetEffectiveUserId() != 0){
        std::cout << "U should be root !" << std::endl;
        return 1;
    }
    else
    {
        Atr atr;
        std::string fullpath = atr.logpath + atr.logfile;
        atr.Obj.fd = open(fullpath.c_str(), O_CREAT | O_RDWR | O_APPEND, 0644);
        if (atr.Obj.fd < 0)
        {
            std::cerr << "Can't open log file for logging. Exiting." << std::endl;
            return 1;
        }
        atr.Obj.Log("Matt_daemon: Started.");
        if (!atr.CreateLockFile())
        {
            atr.Obj.Log("Matt_daemon: Error file locked.", ERROR);
            atr.Obj.Log("Matt_daemon: Quitting.");
            std::cerr << "Can't open :/var/lock/matt_daemon.lock, Daemon already running..." << std::endl;
            close(atr.Obj.fd);
            return 1;
        }
        close(atr.Obj.fd);
        atr.Obj.fd = 0;
        atr.Daemon(); 
    }
    return 0;
}