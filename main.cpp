#include "matt-daemon.hpp"









int main(){

    if (GetEffectiveUserId() != 0){
        std::cout << "U should be root !" << std::endl;
        return 1;
    }
    else
    {
        Atr atr;
        std::cout << "Starting Daemon..." << std::endl;
        atr.Daemon(); 
    }
    return 0;
}