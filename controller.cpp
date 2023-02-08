#include "system.hpp"

void System::supervise_the_machine(const std::string &name) {

    while (true) {
        std::unique_lock<std::mutex> lock(mut_production_for_controller[name]);

        std::unique_lock<std::mutex> lock_prod(mut_production[name]);

        std::pair<product_recipient, machine_controller> mutexes = queue_to_machine[name].front();
        queue_to_machine[name].pop_front();

        mutexes.first.unlock();
        // wątek jakiegoś pracownika wywołuje getProduct
        mutexes.second.lock();

//        TODO: sprawdzenie flagi

        if (!queue_to_machine[name].empty()) {
            lock.unlock();
        }

        lock_prod.unlock();

        break; // TODO: powinna byc nieskonczona z warunkiem
    }
}
