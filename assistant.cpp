#include "system.hpp"


void System::pick_up_product(unsigned int id_employee,
                             std::promise<std::unique_ptr<Product>> &promise,
                             const std::string &name,
                             std::shared_ptr<Machine> &machine) {

    (void) id_employee;
//
//    if (id_employee%2 == 0) {
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//    }

    // tu watki moga zostac wyprzedzone
    std::unique_lock<std::mutex> lock_assistant(mut_assistant[name]);
//    std::cout << "wa: czekam, " << name << ", przez: " << id_employee << "mozna: " << bool_assistant[name] << '\n';

//    std::this_thread::sleep_for(std::chrono::seconds(3));
    cv_assistant[name].wait(lock_assistant, [this, name] {
        if (bool_assistant[name]) {
            bool_assistant[name] = false;
            return true;
        }

        return false;
    });

    if (!machine_closed[name]) {
        try {
//        std::cout << "wa: przeszedlem, " << name << ", przez: " << id_employee << '\n';
            promise.set_value(machine->getProduct());
//        std::cout << "wa: wyprodukowano, " << name << ", przez: " << id_employee << '\n';
        }
        catch (const MachineFailure &e) {
//            std::cout << "wa: zjebana, " << name << ", przez: " << id_employee << '\n';
            promise.set_value(nullptr);
            // produkt ma zniknac z menu // TODO
            machine_closed[name] = true;
        }
    } else {
        promise.set_value(nullptr);
//        std::cout << "wa: maszyna zepsuta nie?, " << name << ", przez: " << id_employee << '\n';
    }

    lock_assistant.unlock();
//    std::cout << "jestem " << id_employee << name << '\n';
    {
        std::lock_guard<std::mutex> lock_controller(mut_controller[name]);
        bool_controller[name] = true;
    }

    cv_controller[name].notify_one();

//    std::cout << "wa: odchodze, przez: " << id_employee << '\n';
}

void System::return_product(unsigned int id_employee,
                            std::unique_ptr<Product> product,
                            const std::string &name,
                            std::shared_ptr<Machine> &machine) {
    (void) id_employee;
//
//    if (id_employee%2 == 0) {
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//    }

    std::unique_lock<std::mutex> lock_assistant(mut_assistant[name]);
//    std::cout << "re1: czekam, " << name << ", przez: " << id_employee << "mozna: " << bool_assistant[name] << '\n';

//    std::this_thread::sleep_for(std::chrono::seconds(3));
    cv_assistant[name].wait(lock_assistant, [this, name] {
        if (bool_assistant[name]) {
            bool_assistant[name] = false;
            return true;
        }

        return false;
    });

    machine->returnProduct(std::move(product));

    lock_assistant.unlock();
//    std::cout << "re2: zwrocilem " << id_employee << name << '\n';

    {
        std::lock_guard<std::mutex> lock_controller(mut_controller[name]); // nie potrzeba
        bool_controller[name] = true;
    }

    cv_controller[name].notify_one();

//    std::cout << "re3: odchodze, przez: " << id_employee << '\n';
}
