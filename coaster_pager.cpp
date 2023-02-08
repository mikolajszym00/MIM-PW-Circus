#include "system.hpp"


void CoasterPager::wait() const {

    system.cp_mutex[id].lock(); // czy na pewno ma orginalnego mutexa
//    std::cout << "wstalem cp" << "\n";

}










