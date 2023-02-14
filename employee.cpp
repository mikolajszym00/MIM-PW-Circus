#include "system.hpp"

// pracownik wysyla asystentow po produkty do maszyn
std::vector<std::thread>
System::send_threads_to_machines(unsigned int id_employee,
                                 std::vector<std::future<std::unique_ptr<Product>>> &futures,
                                 std::vector<std::promise<std::unique_ptr<Product>>> &promises,
                                 const std::vector<std::string> &required_machines,
                                 machines_t &owned_machines) {

    std::vector<std::thread> threads_to_wait_for;

    // tworzenie obietnic
    for (unsigned int i = 0; i < required_machines.size(); i++) {
        std::promise<std::unique_ptr<Product>> pms;
        std::future<std::unique_ptr<Product>> ftr = pms.get_future();

        promises.push_back(std::move(pms));
        futures.push_back(std::move(ftr));
    }

    unsigned int i = 0;
    for (const std::string &name: required_machines) {
        std::thread t{[id_employee, this, i, &promises, name, &owned_machines] {
            pick_up_product(
                    id_employee,
                    promises[i],
                    name,
                    owned_machines[name]);
        }};

        { // informacja dla kontrolera, ze pojawil sie nowy produkt do obslugi
            std::lock_guard<std::mutex> lock_controller(mut_production_for_controller[name]);
            queue_size[name]++;
        }

        unsigned int num = queue_size[name];

        if (num == 1) {
            cv_production_for_controller[name].notify_one();
        }


        threads_to_wait_for.push_back(std::move(t));
        i++;
    }

    return threads_to_wait_for;
}

// pracownik wysyla asystentow, aby zwrocili produkt do maszyny
std::vector<std::thread>
System::return_products_to_machines(unsigned int id_employee,
                                    unique_products_t products,
                                    const std::vector<std::string> &required_machines,
                                    machines_t &owned_machines,
                                    WorkerReport &report) {
    (void) id_employee;

    std::mutex mut_return;
    std::condition_variable cv_return;

    std::vector<std::thread> threads_to_wait_for;

    std::atomic<unsigned int> returned = 0;
    unsigned int i = 0;
    for (const std::string &name: required_machines) {
        if (!products[i]) {
            if (std::find(report.failedProducts.begin(),
                          report.failedProducts.end(), name) == report.failedProducts.end()) {
                report.failedProducts.push_back(name);
            }

            i++;
            returned++;
            continue;
        }

        // produkty ktore nalezy zwrocic

        std::thread t{[&cv_return, &returned, &products, i, id_employee, this, name, &owned_machines] {
            return_product(
                    id_employee,
                    std::move(products[i]),
                    name,
                    owned_machines[name]);

            returned++;
            cv_return.notify_one();
        }};

        { // informacja dla kontrolera, ze pojawil sie nowy produkt do obslugi
            std::lock_guard<std::mutex> lock_controller(mut_production_for_controller[name]);
            queue_size[name]++;
        }

        unsigned int num = queue_size[name];

        if (num == 1) {
            cv_production_for_controller[name].notify_one();
        }


        threads_to_wait_for.push_back(std::move(t));
        i++;
    }

    // czeka aż każdy wątek zwracający zabierze wartość z products
    std::unique_lock<std::mutex> lock_return(mut_return);
    cv_return.wait(lock_return, [required_machines, &returned] {
        return required_machines.size() == returned;
    });

    return threads_to_wait_for;
}

System::unique_products_t
System::collect_products(std::vector<std::thread> threads_to_wait_for,
                         std::vector<std::future<std::unique_ptr<Product>>> futures) {
    unique_products_t products;

    unsigned int i = 0;
    while (i < threads_to_wait_for.size()) {
        products.push_back(futures[i].get());
        threads_to_wait_for[i].join();
        i++;
    }

    return products;
}

void System::wait_for_return(std::vector<std::thread> threads_to_wait_for) {
    unsigned int i = 0;
    while (i < threads_to_wait_for.size()) {
        threads_to_wait_for[i].join();
        i++;
    }
}

void System::prepare_products_for_picking_up(
        unsigned int id_employee,
        unsigned int id,
        unique_products_t products,
        const std::vector<std::string> &required_machines,
        machines_t &owned_machines,
        WorkerReport &report) {

    completed_meals[id] = std::move(products);
    orders_status[id] = Status::ready;
    is_ready[id] = true;

    bool_coaster_pager[id] = true;

    cv_coaster_pager[id].notify_one(); // klient moze oddac pager

    // pracownik zasypia w oczekiwaniu na klienta/ koniec czasu na odebranie
    std::unique_lock<std::mutex> lock(mut_sleep[id]);
    cv_sleep[id].wait_for(lock,
                          std::chrono::milliseconds(clientTimeout),
                          [this, &id]() { return bool_sleep[id]; });

    // system i pracowik probuja zmienic status, w zaleznosci od wyniku produkt zostanie
    // zwrocony lub przekazany klientowi
    bool changed = orders_status.check_and_change(id, Status::ready, Status::expired);

    if (!changed) {
        orders_status.erase(id);
        report.collectedOrders.push_back(required_machines);
        return;
    }

    mut_sleep.erase(id); // jesli pracownik zmienil to znaczy ze system nie bedzie budzil
    cv_sleep.erase(id); //
    bool_sleep.erase(id); //

    std::vector<std::thread>
            threads_to_wait_for_return =
            return_products_to_machines(id_employee,
                                        std::move(completed_meals[id]),
                                        required_machines,
                                        owned_machines,
                                        report);

    wait_for_return(std::move(threads_to_wait_for_return));
    completed_meals.erase(id); // usuwa ten kto wykorzystal

    report.abandonedOrders.push_back(required_machines);
}

void System::prepare_products_for_returning(
        unsigned int id_employee,
        unsigned int id,
        unique_products_t products,
        const std::vector<std::string> &required_machines,
        machines_t &owned_machines,
        WorkerReport &report) {

    orders_status[id] = Status::breakdown;

    bool_coaster_pager[id] = true;
    cv_coaster_pager[id].notify_one(); // klient moze oddac pager

    std::vector<std::thread> threads_to_wait_for_return =
            return_products_to_machines(id_employee,
                                        std::move(products),
                                        required_machines,
                                        owned_machines,
                                        report);

    wait_for_return(std::move(threads_to_wait_for_return));

    report.failedOrders.push_back(required_machines);
}

// glowan petla, w ktorej pracownik zbiera i czeka na zamowienia
void System::work(machines_t &owned_machines, unsigned int id_employee, WorkerReport &report) {
    while (true) {
        // oczekiwanie na nowe zamowienie

        std::unique_lock<std::mutex> lock_emp(mut_ordering_for_employees);

        cv_ordering_for_employees.wait(lock_emp, [this] {
            return (orders_num > 0 || system_closed);
        });

        std::unique_lock<std::mutex> lock(mut_ordering);

        if (system_closed && orders.empty()) {
            cv_ordering_for_employees.notify_one();
            return;
        }

        std::vector<std::future<std::unique_ptr<Product>>> futures;
        std::vector<std::promise<std::unique_ptr<Product>>> promises;

        std::pair<unsigned int, std::vector<std::string>> order = orders.front();
        orders.pop_front();

        orders_num--;
        lock_emp.unlock();

        if (!orders.empty()) {
            cv_ordering_for_employees.notify_one();
        }

        // realizownanie zamowienia

        std::vector<std::thread> threads_to_wait_for =
                send_threads_to_machines(id_employee,
                                         futures,
                                         promises,
                                         order.second,
                                         owned_machines);

        lock.unlock();

        // zbieranie z maszyn produktow
        unique_products_t products =
                collect_products(std::move(threads_to_wait_for),
                                 std::move(futures));

        bool all_delivered = true;
        for (std::unique_ptr<Product> &prod: products) {
            if (!prod) {
                all_delivered = false;
            } else {

            }
        }

        // przygotowywanie odbioru lub zwrot
        if (all_delivered) {
            prepare_products_for_picking_up(id_employee, order.first, std::move(products),
                                            order.second, owned_machines, report);
        } else {
            prepare_products_for_returning(id_employee, order.first, std::move(products),
                                           order.second, owned_machines, report);
        }

    }
}
