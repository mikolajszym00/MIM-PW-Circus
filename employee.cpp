#include "system.hpp"

void System::pick_up_product(
        std::mutex &mut_product_recipient,
        std::mutex &mut_machine_controller,
        std::unique_ptr<Product> &prod,
        const std::string &name,
        const std::shared_ptr<Machine> &machine) {

    mut_product_recipient.lock();

    try {
        prod = machine->getProduct();
    }
    catch (const MachineFailure &e) {
        prod = nullptr;
        machine_closed[name] = true; // TODO: cos na sprawdzenie tej zmiennej
    }

    mut_machine_controller.unlock();
}

std::vector<std::thread> System::send_threads_to_machines(
        std::vector<std::unique_ptr<Product>> products, // TODO: czy z wektorem bedzie dzialac
        const std::vector<std::string> &required_machines,
        machines_t owned_machines) {

    std::vector<std::thread> threads_to_wait_for;

    unsigned int i = 0;
    for (const std::string &name: required_machines) {
        std::mutex mut_product_recipient;
        // zatrzymuje swój wątek przed wywołaniem getProduct
        mut_product_recipient.lock();

        std::mutex mut_machine_controller;
        // zatrzymuje kontrolera przed wpuszceniem kolejnego wątku do getProduct
        mut_machine_controller.lock();

        std::thread t{[&mut_product_recipient, &mut_machine_controller, this, i, &products, name, &owned_machines] {
            pick_up_product(mut_product_recipient,
                            mut_machine_controller,
                            products[i],
                            name,
                            owned_machines[name]);
        }};

        // sprawdzic czy maszyna dziala
        std::unique_lock<std::mutex> lock(mut_production[name]);
        {
            queue_to_machine[name].push_back({mut_product_recipient, mut_machine_controller});

            if (queue_to_machine[name].size() == 1) {
                mut_production_for_controller[name].unlock();
            }
        }
        lock.unlock();

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

void System::work(const machines_t &owned_machines) {
    while (true) {
        std::lock_guard<std::mutex> lock(mut_ordering_for_employees); // TODO unique
        std::lock_guard<std::mutex> lock_ord(mut_ordering);

        std::pair<unsigned int, std::vector<std::string>> order = orders.front();
        orders.pop_front();

        if (!orders.empty()) {
            mut_ordering_for_employees.unlock();
        }

        std::vector<std::unique_ptr<Product>> products; // TODO moze zmienic na wektor
        std::vector<std::thread> threads_to_wait_for = send_threads_to_machines(products, order.second, owned_machines);

        mut_ordering.unlock();

        // zbieranie z maszyn produktow
        collect_products(threads_to_wait_for);

        // sprawdzenie czy są wszystkie produkty
        bool all_delivered = true; // TODO wywalic do funkcji
        for (std::unique_ptr<Product>& prod: products) {
            if (!prod) {
                all_delivered = false;
            }
        }

        if (all_delivered) {
            std::unique_lock<std::mutex> lock_comp(mut_completed_meals);
            completed_meals.emplace(order.first, products);
            lock_comp.unlock();

            mut_coaster_pager[order.first].unlock();
        }
        else {
            // podnieś wyjątek na wait

//            return_products();
        }

        break; // tmp
    }
}
