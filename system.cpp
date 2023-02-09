#include "system.hpp"


std::vector<WorkerReport> System::shutdown() {
    system_closed = true;

    return {};
}

std::vector<std::string> System::getMenu() const {
    if (system_closed) {
        return {};
    }
    std::vector<std::string> keys;
    keys.reserve(machines.size());

    for(const auto& kv : machines) {
        keys.push_back(kv.first);
    }

    return keys;
}

std::vector<unsigned int> System::getPendingOrders() const {
    std::vector<unsigned int> vec;

    for (auto status: orders_status) {
        if (status.second == Status::pending) {
            vec.push_back(status.first);
        }
    }

    return vec;
}

void System::check_products(const std::vector<std::string> &products) {
    for (const std::string& s: products) { // TODO sprawdzic czy maszyna dziala
        if (!machines.count(s))
            throw BadOrderException();
    }
}

std::unique_ptr<CoasterPager> System::order(const std::vector<std::string>& products) {
    if (system_closed) {
        throw RestaurantClosedException();
    }

    check_products(products);

    std::lock_guard<std::mutex> lock(mut_ordering);
    std::unique_ptr<CoasterPager> cp = std::make_unique<CoasterPager>(free_id, *this);

    orders.emplace_back(free_id, products);
    orders_status[free_id] = Status::pending;

    std::mutex mut;
    mut.lock(); // system bierze mutexa, zwolni go dopiero gdy posiłek bedzie gotowy
    // czy w przypadku wyjatkow to jest bezpieczne??
    mut_coaster_pager.insert({free_id, mut});

//    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
//    std::cout << "wstalem system" << "\n";
//    mut.unlock();

    free_id++;

    if (orders.size() == 1) {
        mut_ordering_for_employees.unlock();
    }

//    mut_ordering.unlock(); // guard zwolni mut_ordering??

    return cp;
}

std::vector<std::unique_ptr<Product>> System::collectOrder(std::unique_ptr<CoasterPager> CoasterPager) {
    unsigned int id = CoasterPager->getId();

    if (orders_status.find(id) == orders_status.end()) {
        throw BadPagerException();
    }

    if (orders_status[id] == Status::breakdown) {
        throw FulfillmentFailure();
    }

    if (orders_status[id] == Status::pending) {
        throw OrderNotReadyException();
    }

    std::unique_lock<std::mutex> lock_comp(mut_completed_meals); // TODO to bedzie bezpieczna mapa
    std::vector<std::unique_ptr<Product>> meal = completed_meals[id];
    lock_comp.unlock();

    if (orders_status[id] == Status::expired) {
        throw OrderExpiredException();
    }

    return meal;
}

unsigned int System::getClientTimeout() const {
    return clientTimeout;
}
