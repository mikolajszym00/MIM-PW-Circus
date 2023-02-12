#include "system.hpp"

void System::supervise_the_machine(const std::string &name) {
    while (true) {
        std::unique_lock<std::mutex> lock(mut_production_for_controller[name]);
//        std::cout << "kontroler: mut_prod czekam: " << name  << '\n';

        cv_production_for_controller[name].wait(lock, [this, name] {
//            std::cout << "kont: sprawdzam: " << queue_size[name]  << '\n';
            return (queue_size[name] > 0 || employees_joined); // || machine_closed[name]
        });

        // jesli maszyna zepsuta to idzie do domu
//        if (machine_closed[name]) {
//            cv_recipient[name].notify_all(); // tu nikt nie moze czekac
//            return;
//        }
//        std::unique_lock<std::mutex> lock_prod(mut_production[name]);

        if (employees_joined && queue_size[name] == 0) {
//            std::cout << "k: umieram" << queue_size[name]<< '\n';
            return;
        }

//        std::cout << "queuesize: " << queue_to_machine[name].size() << '\n';
//        std::pair<security, security> &securities = queue_to_machine[name].front();
//        queue_to_machine[name].pop_front();

        {
//            std::cout <<  "czy zgoda: " << bool_recipient[name] << "\n";
            std::lock_guard<std::mutex> lock_recipient(mut_recipient[name]);
            bool_recipient[name] = true;
//            std::cout << "kontroler: zezwol na produkcje: " << name  << '\n';
        }
        cv_recipient[name].notify_one();

        // wątek jakiegoś pracownika wywołuje getProduct

        std::unique_lock<std::mutex> lock_controller(mut_controller[name]);
        cv_controller[name].wait(lock_controller, [this, name] {
            return (bool_controller[name]);
        });

        bool_controller[name] = false;
        lock_controller.unlock(); // to juz nie jest potrzebne

//        queue_to_machine[name].pop_front(); // nie moze usunac danemu watkowi zanim on nie powie ze skonczyl
        queue_size[name]--;

//        std::cout << "kontroler: produkcja zakonczona: " << name  << '\n';

//        if (!(queue_size[name] == 0)) {
//            cv_production_for_controller[name].notify_one();
//        }

        lock.unlock();
//        lock_prod.unlock();
    }
}
