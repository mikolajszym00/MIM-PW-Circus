#include "system.hpp"


void CoasterPager::wait() const {

    system.mut_coaster_pager[id].lock(); // czy na pewno ma orginalnego mutexa
    std::cout << "wstalem cp" << "\n";

    // flaga czy sie powiodlo zamowienie
}

[[nodiscard]] unsigned int CoasterPager::getId() const {
    return id;
}

[[nodiscard]] bool CoasterPager::isReady() const {
    return is_ready;
}







