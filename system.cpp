#include "system.hpp"

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

    for (const auto &machine: machines) { // stops machines
        machine.second->stop();
    }

    std::vector<WorkerReport> wr;

    for (unsigned int j = 0; j < numberOfWorkers; j++) {
        wr.push_back(std::move(reports[j]));
    }

    return wr;
}

std::vector<std::string> System::getMenu() const {
    if (system_closed) {
        return {};
    }
    std::vector<std::string> keys;
    keys.reserve(machines.size());

    unsigned int i = 0;
    for (const auto &kv : machines) {
        if (!machine_closed_get_menu[i]) {
            keys.push_back(kv.first);
        }
    }

    return keys;
}

std::vector<unsigned int> System::getPendingOrders() const {
    std::vector<unsigned int> vec;

//    for (auto status: orders_status) {
//        if (status.second == Status::pending) {
//            vec.push_back(status.first);
//        }
//    }

    return vec;
}

void System::check_products(const std::vector<std::string> &products) {
    for (const std::string& s: products) {
        if (!machines.count(s) || machine_closed[s])
            throw BadOrderException();
    }
}

std::unique_ptr<CoasterPager> System::order(const std::vector<std::string>& products) {
    // wstawia zamowienie do listy (wyjmie je ktorys z pracownikow)
    std::unique_lock<std::mutex> lock(mut_ordering);

    if (system_closed) {
        throw RestaurantClosedException();
    }

    check_products(products);

    std::unique_ptr<CoasterPager> cp = std::make_unique<CoasterPager>(free_id, *this);

    orders.emplace_back(free_id, products);

    orders_status[free_id] = Status::pending;

    free_id++;

    unsigned int num = orders.size();
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
    mut_coaster_pager.erase(id);
    cv_coaster_pager.erase(id);
    is_ready.erase(id);
    bool_coaster_pager.erase(id);
}

std::vector<std::unique_ptr<Product>> System::collectOrder(std::unique_ptr<CoasterPager> CoasterPager) {
    unsigned int id = CoasterPager->getId();

    if (!orders_status.contains(id)) {
        throw BadPagerException();
    }

    if (orders_status[id] == Status::breakdown) {
        throw FulfillmentFailure();
    }

    if (orders_status[id] == Status::pending) {
        throw OrderNotReadyException();
    }

    // system albo dany pracownik moze zmienic status zamowienia (collected lub expired)
    bool changed = orders_status.check_and_change(id, Status::ready, Status::collected);

    if (!changed) {
        orders_status.erase(id); // usuwa ten kto byl drugi
        clean_after_order(id);
        throw OrderExpiredException();
    }

    {
        std::lock_guard<std::mutex> lock(mut_sleep[id]);
        bool_sleep[id] = true;
    }
    cv_sleep[id].notify_one();

//    mut_sleep.erase(id); // pracownik zdejmie locka dopiero jak wyjdzie
//    cv_sleep.erase(id); //
//    bool_sleep.erase(id); //

    unique_products_t meal = std::move(completed_meals[id]);
    completed_meals.erase(id);

    clean_after_order(id);
    return meal;
}

unsigned int System::getClientTimeout() const {
    return clientTimeout;
}
