#include "system.hpp"

void System::supervise_the_machine(const std::string &name) {
    while (true) {
        std::unique_lock<std::mutex> lock(mut_production_for_controller[name]);

        // kontroler czeka az pojawi sie asystent w kolejce
        cv_production_for_controller[name].wait(lock, [this, name] {
            return (queue_size[name] > 0 || employees_joined);
        });

        if (employees_joined && queue_size[name] == 0) {
            return;
        }

        { // kontroler zezwala asystentowi na wejscie
            std::lock_guard<std::mutex> lock_recipient(mut_assistant[name]);
            bool_assistant[name] = true;
        }
        cv_assistant[name].notify_one();

        // asystent korzysta z maszyny ...

        // kontroler czeka na informacje o zakonczeniu pracy przy maszynie
        std::unique_lock<std::mutex> lock_controller(mut_controller[name]);
        cv_controller[name].wait(lock_controller, [this, name] {
            return (bool_controller[name]);
        });

        bool_controller[name] = false;
        lock_controller.unlock();
        queue_size[name]--;

        lock.unlock();
    }
}
