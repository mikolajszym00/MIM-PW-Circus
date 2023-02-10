#include "system.hpp"

void System::supervise_the_machine(const std::string &name) {
    while (true) {
        std::unique_lock<std::mutex> lock(mut_production_for_controller[name]);

        cv_production_for_controller[name].wait(lock, [this, name] {
            return (queue_size[name] > 0 || employees_joined);
        });

        std::unique_lock<std::mutex> lock_prod(mut_production[name]);

        if (system_closed && queue_to_machine.empty()) {
            return;
        }

        std::pair<security &, security &> securities = queue_to_machine[name].front();
        queue_to_machine[name].pop_front();

        security &sec_recipient = securities.first;
        security &sec_controller = securities.second;

        {
            std::lock_guard<std::mutex> lock_recipient(sec_recipient.first.first);
            sec_recipient.second = true;
        }
        sec_recipient.first.second.notify_one();

        // wątek jakiegoś pracownika wywołuje getProduct

        std::unique_lock<std::mutex> lock_controller(sec_controller.first.first);
        sec_controller.first.second.wait(lock_controller, [&sec_controller] { // TODO czy tu &
            return (sec_controller.second);
        });

        sec_controller.second = false;

        if (!queue_to_machine[name].empty()) {
            cv_production_for_controller[name].notify_one();
        }

        lock_prod.unlock();
    }
}
