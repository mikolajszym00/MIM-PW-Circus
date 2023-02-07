#include "system.hpp"


std::vector<WorkerReport> System::shutdown() {
    closed = true;

    return {};
}

std::vector<std::string> System::getMenu() const {
    if (closed) {
        return {};
    }
    std::vector<std::string> keys;
    keys.reserve(machines.size());

    for(auto kv : machines) {
        keys.push_back(kv.first);
    }

    return std::move(keys);
}

void System::check_products(const std::vector<std::string> &products) {
    for (std::string s: products) {
        if (!machines.count(s))
            BadOrderException();
    }
}

std::unique_ptr<CoasterPager> System::order(std::vector<std::string> products) {
    if (closed) {
        RestaurantClosedException();
    }

    check_products(products);

    // nadać numer zamowienia
    std::unique_ptr<CoasterPager> cp = std::make_unique<CoasterPager>();

    std::lock_guard<std::mutex> lock(mut_ordering);
    orders.push_back(std::make_pair(&cp, products));

    if (orders.size() == 1) {
        mut_ordering_for_employees.unlock();
    }

    mut_ordering.unlock();

    return std::move(cp); // czy nie zdejmie ref z cp
}
