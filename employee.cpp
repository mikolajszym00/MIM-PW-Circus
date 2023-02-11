#include "system.hpp"

void System::pick_up_product(unsigned int id_employee,
//                             std::pair<security, security> &se,
                             std::promise<std::unique_ptr<Product>> &promise,
                             const std::string &name,
                             std::shared_ptr<Machine> &machine) {
//    security &security_recipient = se.first;
//    security &security_controller = se.second;

    (void) id_employee;

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

    try {
//        std::cout << "wa: przeszedlem, " << name << ", przez: " << id_employee << '\n';
//        (void ) machine;
//        std::unique_ptr<Product>
//        (void) promise;


//        std::this_thread::sleep_for(std::chrono::seconds(3));
//        machine->getProduct();
        promise.set_value(machine->getProduct());
//        prod = static_cast<std::unique_ptr<Product, std::default_delete<Product>> &&>(p);
//        (void) machine;
//        std::cout << "wa: wyprodukowano, " << name << ", przez: " << id_employee << '\n';
    }
    catch (const MachineFailure &e) {
        promise.set_value(nullptr);
        machine_closed[name] = true;
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

std::vector<std::thread> System::send_threads_to_machines(unsigned int id_employee,
                                                          std::vector<std::future<std::unique_ptr<Product>>> &futures,
                                                          std::vector<std::promise<std::unique_ptr<Product>>> &promises,
                                                          const std::vector<std::string> &required_machines,
                                                          machines_t &owned_machines) {

    std::vector<std::thread> threads_to_wait_for;

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


//        lock.unlock();
//        std::pair<security, security> se;
//        se.first.second = false;
//        se.second.second = false;

//        queue_to_machine[name].emplace_back(std::move(se));

        std::thread t{[id_employee, this, i, &promises, name, &owned_machines] {
            pick_up_product(
                    id_employee,
//                    queue_to_machine[name].back(),
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

void System::collect_products(std::vector<std::thread> threads_to_wait_for,
                              std::vector<std::future<std::unique_ptr<Product>>> futures) {
    unsigned int i = 0;

    while (i < threads_to_wait_for.size()) {
        futures[i].get();
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
        std::vector<std::future<std::unique_ptr<Product>>> futures;
        std::vector<std::promise<std::unique_ptr<Product>>> promises;
        std::vector<std::thread> threads_to_wait_for = send_threads_to_machines(id_employee,
                                                                                futures,
                                                                                promises,
                                                                                order.second,
                                                                                owned_machines); // nie wyciegne tych produktow

//        std::this_thread::sleep_for(std::chrono::seconds(3));
        lock.unlock();
//        std::cout << "p: czekam az zrobia " << '\n';
        // zbieranie z maszyn produktow
        collect_products(std::move(threads_to_wait_for), std::move(futures));

//        futures[i].get(); // przed join było w pliku

//        std::cout << "tak" << "\n";
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
