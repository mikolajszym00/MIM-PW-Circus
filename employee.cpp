#include "system.hpp"

void System::pick_up_product(unsigned int id_employee,
                             std::promise<std::unique_ptr<Product>> &promise,
                             const std::string &name,
                             std::shared_ptr<Machine> &machine) {

    (void) id_employee;

//    if (id_employee%2 == 0) {
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//    }

    // tu watki moga zostac wyprzedzone
    std::unique_lock<std::mutex> lock_recipient(mut_recipient[name]);
//    std::cout << "wa: czekam, " << name << ", przez: " << id_employee << "mozna: " << bool_recipient[name] << '\n';

//    std::this_thread::sleep_for(std::chrono::seconds(3));
    cv_recipient[name].wait(lock_recipient, [this, name] {
        if (bool_recipient[name]) {
            bool_recipient[name] = false;
            return true;
        }

        return false;
    });

    if (!machine_closed[name]) {
        try {
//        std::cout << "wa: przeszedlem, " << name << ", przez: " << id_employee << '\n';
            promise.set_value(machine->getProduct());
//        std::cout << "wa: wyprodukowano, " << name << ", przez: " << id_employee << '\n';
        }
        catch (const MachineFailure &e) {
//            std::cout << "wa: zjebana, " << name << ", przez: " << id_employee << '\n';
            promise.set_value(nullptr);
            // produkt ma zniknac z menu // TODO
            machine_closed[name] = true;
        }
    } else {
        promise.set_value(nullptr);
//        std::cout << "wa: maszyna zepsuta nie?, " << name << ", przez: " << id_employee << '\n';
    }

    lock_recipient.unlock();
//    std::cout << "jestem " << id_employee << name << '\n';
    {
        std::lock_guard<std::mutex> lock_controller(mut_controller[name]);
        bool_controller[name] = true;
    }

    cv_controller[name].notify_one();

//    std::cout << "wa: odchodze, przez: " << id_employee << '\n';
}

void System::return_product(unsigned int id_employee,
                            std::unique_ptr<Product> product,
                            const std::string &name,
                            std::shared_ptr<Machine> &machine) {
    (void) id_employee;

    std::unique_lock<std::mutex> lock_recipient(mut_recipient[name]);
//    std::cout << "re: czekam, " << name << ", przez: " << id_employee << "mozna: " << bool_recipient[name] << '\n';

//    std::this_thread::sleep_for(std::chrono::seconds(3));
    cv_recipient[name].wait(lock_recipient, [this, name] {
        if (bool_recipient[name]) {
            bool_recipient[name] = false;
            return true;
        }

        return false;
    });

    machine->returnProduct(std::move(product));

    lock_recipient.unlock();
//    std::cout << "jestem " << id_employee << name << '\n';

    {
        std::lock_guard<std::mutex> lock_controller(mut_controller[name]); // nie potrzeba
        bool_controller[name] = true;
    }

    cv_controller[name].notify_one();

//    std::cout << "wa: odchodze, przez: " << id_employee << '\n';
}

std::vector<std::thread>
System::send_threads_to_machines(unsigned int id_employee,
                                 std::vector<std::future<std::unique_ptr<Product>>> &futures,
                                 std::vector<std::promise<std::unique_ptr<Product>>> &promises,
                                 const std::vector<std::string> &required_machines,
                                 machines_t &owned_machines) {

    std::vector<std::thread> threads_to_wait_for;
//    std::vector<std::pair<unsigned int, std::string>> machine_id;

    for (unsigned int i = 0; i < required_machines.size(); i++) {
        std::promise<std::unique_ptr<Product>> pms;
        std::future<std::unique_ptr<Product>> ftr = pms.get_future();

        promises.push_back(std::move(pms));
        futures.push_back(std::move(ftr));
    }

    unsigned int i = 0;
    for (const std::string &name: required_machines) {
//        std::cout << "p, name: " << name << " " << "przez: " << id_employee << " i: " << i << "\n";

        // sprawdzic czy maszyna dziala

//        std::cout << "za mut_ordering " << id_employee << ' ' << orders_num << '\n';

//        std::unique_lock<std::mutex> lock(mut_production[name]);

//        std::cout << "p: mut_prod passed " << "maszyna: " << name << " " << "pracownik: " << id_employee << " i: " << i
//                  << "\n";

        std::thread t{[id_employee, this, i, &promises, name, &owned_machines] {
            pick_up_product(
                    id_employee,
                    promises[i],
                    name,
                    owned_machines[name]);
        }};

        {
            std::lock_guard<std::mutex> lock_controller(mut_production_for_controller[name]);
            queue_size[name]++;
        }

        unsigned int num = queue_size[name];

        if (num == 1) {
//            std::cout << "p: on wie" << "\n";
            cv_production_for_controller[name].notify_one();
        }


        threads_to_wait_for.push_back(std::move(t));
        i++;
    }

    return threads_to_wait_for;
}

std::vector<std::thread>
System::return_products_to_machines(unsigned int id_employee,
                                    unique_products_t products,
                                    const std::vector<std::string> &required_machines,
                                    machines_t &owned_machines) {
    (void) id_employee;
    std::vector<std::thread> threads_to_wait_for;

    unsigned int i = 0;
    for (const std::string &name: required_machines) {
        if (!products[i]) {
            i++;
//            std::cout << "r, nulle, name: " << name << " " << "przez: " << id_employee << " i: " << i << "\n";
            continue;
        }

//        std::cout << "r, do zwrotu, name: " << name << " " << "przez: " << id_employee << " i: " << i << "\n";
        // sprawdzic czy maszyna dziala

//        std::cout << "r: za mut_ordering " << id_employee << ' ' << orders_num << '\n';

//        std::unique_lock<std::mutex> lock(mut_production[name]);

//        std::cout << "r: mut_prod passed " << "maszyna: " << name << " " << "pracownik: " << id_employee << " i: " << i
//                  << "\n";

        std::thread t{[id_employee, this, i, &products, name, &owned_machines] {
            return_product(
                    id_employee,
                    std::move(products[i]),
                    name,
                    owned_machines[name]);
        }};

        {
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

void System::work(machines_t &owned_machines, unsigned int id_employee) {
    while (true) {
//        print("stoje przed mut dla emp: ", id_employee);

        std::unique_lock<std::mutex> lock_emp(mut_ordering_for_employees);

//        std::cout << "przed wait: " << id_employee << " " << orders_num << " " << system_closed << '\n';


        cv_ordering_for_employees.wait(lock_emp, [this] {
            return (orders_num > 0 || system_closed);
        });

//        std::cout << "przekroczylem wait: " << id_employee << " " << orders_num << " " << system_closed << '\n';
//        print("przekroczylem wait: ", id_employee);

        std::unique_lock<std::mutex> lock(mut_ordering);
        if (system_closed && orders.empty()) {
            cv_ordering_for_employees.notify_one();
//            std::cout << "e: umieram" << '\n';
            return;
        }

//        std::cout << "za mut_ordering " << id_employee << ' ' << orders_num << " orders size: " << orders.size()
//                  << '\n';

        std::pair<unsigned int, std::vector<std::string>> order = orders.front(); // to jest kopia
        orders.pop_front();

        orders_num--;
        lock_emp.unlock();

        if (!orders.empty()) {
//            mut_ordering_for_employees.unlock();
            cv_ordering_for_employees.notify_one();
        }

        std::vector<std::future<std::unique_ptr<Product>>> futures;
        std::vector<std::promise<std::unique_ptr<Product>>> promises;

        std::vector<std::thread> threads_to_wait_for =
                send_threads_to_machines(id_employee,
                                         futures,
                                         promises,
                                         order.second,
                                         owned_machines);

//        std::this_thread::sleep_for(std::chrono::seconds(3));
        lock.unlock();
//        std::cout << "p: czekam az zrobia " << '\n';
        // zbieranie z maszyn produktow
        unique_products_t products =
                collect_products(std::move(threads_to_wait_for),
                                 std::move(futures));

//        std::cout << "tak" << "\n";

        // sprawdzenie czy są wszystkie produkty
        bool all_delivered = true;
        for (std::unique_ptr<Product> &prod: products) {
            if (!prod) {
                all_delivered = false;
            } else {
//                std::cout << "nazwa prod: " << prod->name << "\n";
            }
        }

        if (all_delivered) {
//            std::cout << "del: all_delivered " << id_employee << "\n";
            completed_meals[order.first] = std::move(products);
            orders_status[order.first] = Status::ready;

            bool_coaster_pager[order.first] = true; // nie trzeba lock guarda ?

            cv_coaster_pager[order.first].notify_one();

//            std::timed_mutex timed_mutex;
            usleep(clientTimeout * 1000);
//            if (timed_mutex.try_lock_for(clientTimeout * 1ms)) { // TODO wczesniejsze budzenie jesli odbierze
//                std::cout << "del: przeszedlem " << id_employee << "\n";
                bool changed = orders_status.check_and_change(order.first, Status::ready, Status::expired);

                if (changed) {
//                    std::unique_lock<std::mutex> lock1(mut_ordering);

                    std::vector<std::thread> threads_to_wait_for_return =
                    return_products_to_machines(id_employee,
                                                std::move(completed_meals[order.first]),
                                                order.second,
                                                owned_machines);

                    wait_for_return(std::move(threads_to_wait_for_return));

//                    lock1.unlock();
                }

//                timed_mutex.unlock();
//            }
//        else {
//                std::cout << "nie dziala" << "\n"; // TODO usun
//                throw BadOrderException(); // TODO usun
//            }
        } else {
//            std::cout << "del: need return " << id_employee << "\n";

            orders_status[order.first] = Status::breakdown;

            bool_coaster_pager[order.first] = true; // nie trzeba lock guarda ?
            cv_coaster_pager[order.first].notify_one();

//            std::unique_lock<std::mutex> lock2(mut_ordering);

            std::vector<std::thread> threads_to_wait_for_return =
                    return_products_to_machines(id_employee,
                                                std::move(products),
                                                order.second,
                                                owned_machines);

            wait_for_return(std::move(threads_to_wait_for_return));

//            std::cout << "del: zebrani " << id_employee << "\n";
//            lock2.unlock();
        }
    }
}
