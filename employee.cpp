#include "system.hpp"

void System::pick_up_product(unsigned int id_employee,
                             std::pair<security, security> &se,
                             std::unique_ptr<Product> &prod,
                             const std::string &name,
                             const std::shared_ptr<Machine> &machine) {
//    (void ) id_employee;
//    (void ) se;
//    (void ) prod;
//    (void) name;
//    (void) machine;
    security &security_recipient = se.first;
    security &security_controller = se.second;

    std::unique_lock<std::mutex> lock_recipient(*security_recipient.first.first);
    std::cout << "wa: czekam, " << name << ", przez: " << id_employee << "mozna: " << security_recipient.second << '\n';

    std::this_thread::sleep_for(std::chrono::seconds(3));
    security_recipient.first.second->wait(lock_recipient, [&security_recipient] {
        return security_recipient.second;
    });
    security_recipient.second = false; // TODO czy tak ma byc, nie musi

    try {
        std::cout << "wa: przeszedlem, " << name << ", przez: " << id_employee << '\n';
        std::unique_ptr<Product> p = machine->getProduct(); // TODO promise i future??
//        prod = static_cast<std::unique_ptr<Product, std::default_delete<Product>> &&>(p);
//        (void) machine;
        std::cout << "wa: wyprodukowano, " << name << ", przez: " << id_employee << '\n';
    }
    catch (const MachineFailure &e) {
        prod = nullptr;
        machine_closed[name] = true;
    }

    lock_recipient.unlock();

    {
        std::lock_guard<std::mutex> lock_controller(*security_controller.first.first);
        security_controller.second = true;
    }

    security_controller.first.second->notify_one();

    std::cout << "wa: odchodze, przez: " << id_employee << '\n';
}

std::vector<std::thread> System::send_threads_to_machines(unsigned int id_employee,
                                                          std::vector<std::unique_ptr<Product>> products, // TODO: czy z wektorem bedzie dzialac
                                                          const std::vector<std::string> &required_machines,
                                                          machines_t owned_machines) {

    std::vector<std::thread> threads_to_wait_for;

    unsigned int i = 0;
    for (const std::string &name: required_machines) {
        std::cout << "p, name: " << name << " " << "przez: " << id_employee << " i: " << i << "\n";

//        std::mutex mut_product_recipient;
//        std::condition_variable cv_product_recipient;


//        security sec_recipient = {std::make_pair(std::move(mut_product_recipient),
//                                                 std::move(cv_product_recipient)), false};

//        security sec_recipient;
//        sec_recipient.second = false;
//
//        security sec_controller;
//        sec_controller.second = false;

//        std::mutex mut_machine_controller;
//        std::condition_variable cv_machine_controller;

//        security sec_controller = {{mut_machine_controller, cv_machine_controller}, false};

        // sprawdzic czy maszyna dziala

//        std::cout << "za mut_ordering " << id_employee << ' ' << orders_num << '\n';

        std::unique_lock<std::mutex> lock(mut_production[name]);

        std::cout << "p: mut_prod passed " << "maszyna: " << name << " " << "pracownik: " << id_employee << " i: " << i
                  << "\n";

        std::pair<security, security> se;
        se.first.second = false;
        se.second.second = false;

        queue_to_machine[name].emplace_back(std::move(se));

        std::thread t{[id_employee, this, i, &products, name, &owned_machines] {
            pick_up_product(
                    id_employee,
                    queue_to_machine[name].back(),
                    products[i],
                    name,
                    owned_machines[name]);
        }};

        {
            std::lock_guard<std::mutex> lock_controller(mut_production_for_controller[name]);
            queue_size[name]++;
        }

        unsigned int num = queue_to_machine[name].size();

        lock.unlock();

        if (num == 1)
            cv_production_for_controller[name].notify_one();

        threads_to_wait_for.push_back(std::move(t));
        i++;
//        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    return threads_to_wait_for;
}

void System::collect_products(std::vector<std::thread> threads_to_wait_for) {
    unsigned int i = 0;


    while (i < threads_to_wait_for.size()) {
        threads_to_wait_for[i].join();
        i++;
    }
}

void System::work(const machines_t &owned_machines, unsigned int id_employee) {
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
            std::cout << "e: umieram" << '\n';
            return;
        }

        std::cout << "za mut_ordering " << id_employee << ' ' << orders_num << " orders size: " << orders.size()
                  << '\n';
//        print("za mut_ordering", id_employee);

        std::pair<unsigned int, std::vector<std::string>> order = orders.front();
        orders.pop_front();

        orders_num--;
        lock_emp.unlock();

        if (!orders.empty()) {
//            mut_ordering_for_employees.unlock();
            cv_ordering_for_employees.notify_one();
        }
//        (void) order;
        std::vector<std::unique_ptr<Product>> products; // TODO czy działa jako wektor
        std::vector<std::thread> threads_to_wait_for = send_threads_to_machines(id_employee,
                                                                                std::move(products),
                                                                                order.second,
                                                                                owned_machines); // nie wyciegne tych produktow

//        std::cout << "ok " << '\n';
        lock.unlock();
//        std::cout << "ok " << '\n';
        // zbieranie z maszyn produktow
        collect_products(std::move(threads_to_wait_for));

        std::cout << "tak" << "\n";
//
//        // sprawdzenie czy są wszystkie produkty
//        bool all_delivered = true; // TODO wywalic do funkcji
//        for (std::unique_ptr<Product>& prod: products) {
//            if (!prod) {
//                all_delivered = false;
//            }
//        }
//
//        if (all_delivered) {
//            std::unique_lock<std::mutex> lock_comp(mut_completed_meals); // TODO to bedzie bezpieczna mapa
//            completed_meals.emplace(order.first, products);
//            lock_comp.unlock();
//
//            orders_status[order.first] = Status::ready;
//            mut_coaster_pager[order.first].unlock();
//        }
//        else {
//            orders_status[order.first] = Status::breakdown;
//            mut_coaster_pager[order.first].unlock();
//
////            return_products();
//        }
    }
}
