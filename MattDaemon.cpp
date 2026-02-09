#include "Md_header.hpp"


int main(){

    if (GetEffectiveUserId() != 0){
        std::cout << "U should be root !" << std::endl;
        return 1;
    }
    else
    {
        Atr atr;
        atr.Daemon(); 
    }
    return 0;
}