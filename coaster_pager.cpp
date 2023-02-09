#include "system.hpp"


void CoasterPager::wait() const {

    system.mut_coaster_pager[id].lock(); // czy na pewno ma orginalnego mutexa

    if (system.orders_status[id] == System::Status::breakdown) {
        throw FulfillmentFailure();
    }
}

[[nodiscard]] unsigned int CoasterPager::getId() const {
    return id;
}

[[nodiscard]] bool CoasterPager::isReady() const {
    return is_ready;
}







