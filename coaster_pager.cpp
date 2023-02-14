#include "system.hpp"


void CoasterPager::wait() const {

    std::unique_lock<std::mutex> lock(system.mut_coaster_pager[id]);

    system.cv_coaster_pager[id].wait(lock, [this] {
        return system.bool_coaster_pager[id];
    });

    if (system.orders_status[id] == System::Status::breakdown) {
        throw FulfillmentFailure();
    }
}

void CoasterPager::wait(unsigned int timeout) const {
    usleep(timeout * 1000);
}

[[nodiscard]] unsigned int CoasterPager::getId() const {
    return id;
}

[[nodiscard]] bool CoasterPager::isReady() const {
    return system.is_ready.at(getId());
}
