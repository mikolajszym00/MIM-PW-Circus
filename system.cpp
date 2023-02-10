#include "system.hpp"


void System::print(const std::string &s, unsigned int id) {
    std::lock_guard<std::mutex> lock(mut_print);
    std::cout << s << id << "\n";
}

std::vector<WorkerReport> System::shutdown() {
    system_closed = true;
    cv_ordering_for_employees.notify_one();

    unsigned int i = 0;
    while (i < threads_employees.size()) {
        threads_employees[i].join();
        i++;
    }

    employees_joined = true;

    for (const auto& machine: machines) {
        cv_production_for_controller[machine.first].notify_one();
    }

    i = 0;
    while (i < threads_controllers.size()) {
        threads_controllers[i].join();
        i++;
    }

    // wylaczyc maszyny

    // zebranie raportow

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
    std::unique_lock<std::mutex> lock(mut_ordering);

    if (system_closed) {
        throw RestaurantClosedException();
    }

    check_products(products);

    std::unique_ptr<CoasterPager> cp = std::make_unique<CoasterPager>(free_id, *this);

    orders.emplace_back(free_id, products);
    orders_status[free_id] = Status::pending;

//    std::mutex mut;
//    mut.lock(); // system bierze mutexa, zwolni go dopiero gdy posiłek bedzie gotowy
    // czy w przypadku wyjatkow to jest bezpieczne??


//    mut_coaster_pager.insert({free_id, mut});

//    print("free_id dodano: ", free_id);
    free_id++;

    unsigned int num = orders.size();
//    std::cout << num << " ile" << "\n";

    lock.unlock();

    {
        std::lock_guard<std::mutex> lock_ord(mut_ordering_for_employees);
        orders_num++;
    }

    if (num == 1)
        cv_ordering_for_employees.notify_one();

    return cp;
}

void System::clean_after_order(unsigned int id) {
    orders_status.erase(id);
    mut_coaster_pager.erase(id);
    completed_meals.erase(id);
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
    std::vector<std::unique_ptr<Product>> meal = std::move(completed_meals[id]);
    lock_comp.unlock();

    if (orders_status[id] == Status::expired) { // todo sprawdzac czas wzgledem wydania i dopiero potem zmieniac status
        throw OrderExpiredException();
    }

    clean_after_order(id);

    return meal;
}

unsigned int System::getClientTimeout() const {
    return clientTimeout;
}
