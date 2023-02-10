#include "system.hpp"

void System::pick_up_product(
        security &security_recipient,
        security &security_controller,
        std::unique_ptr<Product> &prod,
        const std::string &name,
        const std::shared_ptr<Machine> &machine) {

    std::unique_lock<std::mutex> lock_recipient(security_recipient.first.first);
    security_recipient.first.second.wait(lock_recipient, [&security_recipient] {
        return (security_recipient.second);
    });

    security_recipient.second = false; // TODO czy tak ma byc

    try {
        prod = machine->getProduct();
    }
    catch (const MachineFailure &e) {
        prod = nullptr;
        machine_closed[name] = true; // TODO: cos na sprawdzenie tej zmiennej
    }

    lock_recipient.unlock();

    {
        std::lock_guard<std::mutex> lock_controller(security_controller.first.first);
        security_controller.second = true;
    }
    security_controller.first.second.notify_one();
}

std::vector<std::thread> System::send_threads_to_machines(
        std::vector<std::unique_ptr<Product>> products, // TODO: czy z wektorem bedzie dzialac
        const std::vector<std::string> &required_machines,
        machines_t owned_machines) {

    std::vector<std::thread> threads_to_wait_for;

    unsigned int i = 0;
    for (const std::string &name: required_machines) {
        std::mutex mut_product_recipient;
        std::condition_variable cv_product_recipient;

        security sec_recipient = {{mut_product_recipient, cv_product_recipient}, false};

        std::mutex mut_machine_controller;
        std::condition_variable cv_machine_controller;

        security sec_controller = {{mut_machine_controller, cv_machine_controller}, false};

        std::thread t{[&sec_recipient, &sec_controller, this, i, &products, name, &owned_machines] {
            pick_up_product(sec_recipient,
                            sec_controller,
                            products[i],
                            name,
                            owned_machines[name]);
        }};

        // sprawdzic czy maszyna dziala


        std::unique_lock<std::mutex> lock(mut_production[name]);

        queue_to_machine[name].push_back({sec_recipient, sec_controller});

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
        (void) id_employee;
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
            return;
        }

//        std::cout << "za mut_ordering " << id_employee << ' ' << orders_num << '\n';
//        print("za mut_ordering", id_employee);

        std::pair<unsigned int, const std::vector<std::string> &> order = orders.front();
        (void) order;
        orders.pop_front();

        orders_num--;
        lock_emp.unlock();

        if (!orders.empty()) {
//            mut_ordering_for_employees.unlock();
            cv_ordering_for_employees.notify_one();
        }

        std::vector<std::unique_ptr<Product>> products; // TODO czy działa jako wektor
        std::vector<std::thread> threads_to_wait_for = send_threads_to_machines(products, order.second, owned_machines);

        (void) owned_machines;
        lock.unlock();
//
//        // zbieranie z maszyn produktow
//        collect_products(threads_to_wait_for);
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
