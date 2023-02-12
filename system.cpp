#include "system.hpp"

long long System::get_curr_time_in_millis() {
    std::chrono::time_point<std::chrono::system_clock> now =
            std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    return millis;
}

std::vector<WorkerReport> System::shutdown() {
    system_closed = true;

    //oni moga jeszcze zwracac??
    cv_ordering_for_employees.notify_one();
//    std::cout << "zamykamy" << '\n';

    unsigned int i = 0;
    while (i < threads_employees.size()) {
        threads_employees[i].join();
        i++;
    }
//    std::cout << "emplo joinde" << '\n';
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

    // zebranie raportow

    return {};
}

std::vector<std::string> System::getMenu() const {
    if (system_closed) {
        return {};
    }
    std::vector<std::string> keys;
    keys.reserve(machines.size());

    for (const auto &kv : machines) {
//        machine_closed[kv.first]; sprawdzic czy zamkneta- nie mozna bo nie jest const
        keys.push_back(kv.first);
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
    std::unique_lock<std::mutex> lock(mut_ordering);

    if (system_closed) {
        throw RestaurantClosedException();
    }

    check_products(products);

    std::unique_ptr<CoasterPager> cp = std::make_unique<CoasterPager>(free_id, *this);

    orders.emplace_back(free_id, products);
    orders_status[free_id] = Status::pending;

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

    if (!orders_status.contains(id)) {
        throw BadPagerException();
    }

    if (orders_status[id] == Status::breakdown) {
        throw FulfillmentFailure();
    }

    if (orders_status[id] == Status::pending) {
        throw OrderNotReadyException();
    }
//    std::this_thread::sleep_for(std::chrono::seconds(3));
    bool changed = orders_status.check_and_change(id, Status::ready, Status::collected);

    if (!changed) {
        std::cout << "przeterminowne" << "\n";
        clean_after_order(id);
        throw OrderExpiredException();
    }

    unique_products_t meal = std::move(completed_meals[id]);

    clean_after_order(id);
    return meal;
}

unsigned int System::getClientTimeout() const {
    return clientTimeout;
}
