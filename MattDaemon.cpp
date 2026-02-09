#include "Md_header.hpp"


int main(){

    if (GetEffectiveUserId() != 0){
        std::cout << "U should be root !" << std::endl;
        return 1;
    }
    else
    {
        Atr atr;
        if (!atr.CreateLockFile())
        {
             std::cerr << "Can't open :/var/lock/matt_daemon.lock, Daemon already running..." << std::endl;
             return 1;
        }
        atr.Daemon(); 
    }
    return 0;
}