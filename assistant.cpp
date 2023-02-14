#include "system.hpp"


void System::pick_up_product(unsigned int id_employee,
                             std::promise<std::unique_ptr<Product>> &promise,
                             const std::string &name,
                             std::shared_ptr<Machine> &machine) {

    (void) id_employee;

    std::unique_lock<std::mutex> lock_assistant(mut_assistant[name]);

    // asystent czeka az zostanie wpuszczony do maszyny
    cv_assistant[name].wait(lock_assistant, [this, name] {
        if (bool_assistant[name]) {
            bool_assistant[name] = false;
            return true;
        }

        return false;
    });

    if (!machine_closed[name]) {
        try {
            promise.set_value(machine->getProduct());
        }
        catch (const MachineFailure &e) { // spichlerz jest pusty
            promise.set_value(nullptr);
            unsigned int id = machine_name_to_id[name];
            machine_closed_get_menu[id] = true;
            machine_closed[name] = true;
        }
    } else {
        promise.set_value(nullptr);
    }

    lock_assistant.unlock();

    { // asystent powiadamia kontrolera ze skonczyl korzystac z maszyny
        std::lock_guard<std::mutex> lock_controller(mut_controller[name]);
        bool_controller[name] = true;
    }

    cv_controller[name].notify_one();
}

void System::return_product(unsigned int id_employee,
                            std::unique_ptr<Product> product,
                            const std::string &name,
                            std::shared_ptr<Machine> &machine) {
    (void) id_employee;

    std::unique_lock<std::mutex> lock_assistant(mut_assistant[name]);

    // asystent czeka az zostanie wpuszczony do maszyny
    cv_assistant[name].wait(lock_assistant, [this, name] {
        if (bool_assistant[name]) {
            bool_assistant[name] = false;
            return true;
        }

        return false;
    });

    machine->returnProduct(std::move(product));

    lock_assistant.unlock();

    { // asystent powiadamia kontrolera ze skonczyl korzystac z maszyny
        std::lock_guard<std::mutex> lock_controller(mut_controller[name]);
        bool_controller[name] = true;
    }

    cv_controller[name].notify_one();
}
