#include "system.hpp"


void get_product_from_machine() {

}


std::vector<std::thread> send_threads_to_machines
    (const std::vector<std::string>& required_machines) { // dodac do hpp
    std::vector<std::thread> threads_to_wait_for;

    for (std::string name: required_machines) {

    }


    return threads_to_wait_for;
}

void work(std::mutex& mut_ordering, std::mutex& mut_ordering_for_employees,
          std::list<std::pair<unsigned int, std::vector<std::string>>>& orders) {


    while (true) {
        std::lock_guard<std::mutex> lock(mut_ordering_for_employees);

        std::lock_guard<std::mutex> lock_ord(mut_ordering);

        std::pair<unsigned int, std::vector<std::string>> order = orders.front();
        orders.pop_front();

        if (!orders.empty()) {
            mut_ordering_for_employees.unlock();
        }


        send_threads_to_machines(order.second);
        // wysłanie wątków do maszyn

        mut_ordering.unlock();


        // zbieranie z maszyn produktow (zbieranie wyjatkow)
        // ew zwrot produktow
        bool all_collected = collect_products();
//        std::vector<std::unique_ptr<Product>>

        if (!all_collected) {
            // zmien flage na nieudana

            return_products();
        }

        // przygotuj posilek do odebrania lub informacje o niepowodzeniu
        // uwolnij klienta z wait

        break; // tmp
    }
}









