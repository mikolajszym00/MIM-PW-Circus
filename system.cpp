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

    for(const auto& kv : machines) {
        keys.push_back(kv.first);
    }

    return keys;
}

void System::check_products(const std::vector<std::string> &products) {
    for (const std::string& s: products) {
        if (!machines.count(s))
            throw BadOrderException();
    }
}

std::unique_ptr<CoasterPager> System::order(const std::vector<std::string>& products) {
    if (closed) {
        throw RestaurantClosedException();
    }

    check_products(products);

    std::lock_guard<std::mutex> lock(mut_ordering);
    std::unique_ptr<CoasterPager> cp = std::make_unique<CoasterPager>(free_id, *this);

    orders.emplace_back(free_id, products);

    std::mutex mut;
    mut.lock(); // system bierze mutexa, zwolni go dopiero gdy posiłek bedzie gotowy
    // czy w przypadku wyjatkow to jest bezpieczne??
    cp_mutex.insert({free_id, mut});

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

unsigned int System::getClientTimeout() const {
    return clientTimeout;
}
