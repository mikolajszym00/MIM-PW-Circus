#include "system.hpp"


void System::get_product_from_machine(std::unique_ptr<Product>& prod, const std::shared_ptr<Machine>& machine) {
    // łapanie wyjatku

    prod = machine->getProduct();
}

std::vector<std::thread> System::send_threads_to_machines(
        std::unique_ptr<Product>* products,
        const std::vector<std::string>& required_machines,
        machines_t owned_machines) {

    std::vector<std::thread> threads_to_wait_for;

    unsigned int i = 0;
    for (const std::string& name: required_machines) {
        std::thread t{[this, i, &products, name, &owned_machines]{
            get_product_from_machine(products[i], owned_machines[name]);
        }};

        threads_to_wait_for.push_back(std::move(t));

        // dodac do kolejki czekajacych dla watku przy maszynie??
        // cond moze ze ktos jest


        i++;
    }

    return threads_to_wait_for;
}

bool System::collect_products(std::vector<std::thread> threads_to_wait_for) {
    try { // moze niedokonczyc joinowania
        for (std::thread &t: threads_to_wait_for) {
            t.join();
        }
    }
    catch (const MachineFailure& e) {
        std::cout << "znowu to zrobil" << "\n";

        return false;
    }

    return true;
}

void System::work(const machines_t& owned_machines) {
    while (true) {
        std::lock_guard<std::mutex> lock(mut_ordering_for_employees);

        std::lock_guard<std::mutex> lock_ord(mut_ordering);

        std::pair<unsigned int, std::vector<std::string>> order = orders.front();
        orders.pop_front();

        if (!orders.empty()) {
            mut_ordering_for_employees.unlock();
        }

        std::unique_ptr<Product> products[owned_machines.size()];
        std::vector<std::thread> threads_to_wait_for = send_threads_to_machines(products,
                                                                                order.second,
                                                                                owned_machines);

        mut_ordering.unlock();


        // zbieranie z maszyn produktow (zbieranie wyjatkow)
        // ew zwrot produktow
        bool all_collected = collect_products(threads_to_wait_for);

        if (!all_collected) {
            // zmien flage na nieudana

            return_products();
        }

        // przygotuj posilek do odebrania lub informacje o niepowodzeniu
        // uwolnij klienta z wait

        break; // tmp
    }
}


//void work(std::mutex& mut_ordering, std::mutex& mut_ordering_for_employees,
//          std::list<std::pair<unsigned int, std::vector<std::string>>>& orders) {
//
//
//    while (true) {
//        std::lock_guard<std::mutex> lock(mut_ordering_for_employees);
//
//        std::lock_guard<std::mutex> lock_ord(mut_ordering);
//
//        std::pair<unsigned int, std::vector<std::string>> order = orders.front();
//        orders.pop_front();
//
//        if (!orders.empty()) {
//            mut_ordering_for_employees.unlock();
//        }
//
//
//        send_threads_to_machines(order.second);
//        // wysłanie wątków do maszyn
//
//        mut_ordering.unlock();
//
//
//        // zbieranie z maszyn produktow (zbieranie wyjatkow)
//        // ew zwrot produktow
//        bool all_collected = collect_products();
////        std::vector<std::unique_ptr<Product>>
//
//        if (!all_collected) {
//            // zmien flage na nieudana
//
//            return_products();
//        }
//
//        // przygotuj posilek do odebrania lub informacje o niepowodzeniu
//        // uwolnij klienta z wait
//
//        break; // tmp
//    }
//}









