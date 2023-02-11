
#include "test_production.h"

//void sleep(int sec) {
//    std::this_thread::sleep_for(std::chrono::seconds(sec));
//}

class Count {
public:
    Count() = default;

    int i = 0;

    int get_i() {
        i++;
        return i;
    }
};

Count c_p = Count();

void t_p1(int num_clients, unsigned int num_workers, bool sleep1, bool sleep2, bool sleep3) {
    System system{
            {
                    {"cheeseburger", std::shared_ptr<Machine>(new CheeseBurgerMachine())}
            },
            num_workers, 1};

    std::vector<std::thread> threads;

    if (sleep1)
        sleep(1);

    for (int i = 0; i < num_clients; i++) {
        auto client = std::thread([&system]() {
            system.order({"cheeseburger", "cheeseburger"});
        });

        threads.push_back(std::move(client));
    }

    if (sleep2)
        sleep(1);

    for (std::thread &thread: threads) {
        thread.join();
    }

    if (sleep3)
        sleep(1);

//    sleep(10);

    system.shutdown();

    std::cout << "passed" << c_p.get_i() << "\n";
}

void t_p2(const std::vector<std::vector<std::string>> &clients_orders, unsigned int num_workers, bool sleep1, bool sleep2, bool sleep3) {
    System system{
            {
                    {"cheeseburger", std::shared_ptr<Machine>(new CheeseBurgerMachine())},
                    {"hamburger", std::shared_ptr<Machine>(new CheeseBurgerMachine())},
                    {"milkburger", std::shared_ptr<Machine>(new CheeseBurgerMachine())},
                    {"noburger", std::shared_ptr<Machine>(new CheeseBurgerMachine())}
            },
            num_workers, 1};

    std::vector<std::thread> threads;

    if (sleep1)
        sleep(1);

    for (const std::vector<std::string> &client_order: clients_orders) {
        auto client = std::thread([&system, client_order]() {
            system.order(client_order);
        });

        threads.push_back(std::move(client));
    }

    if (sleep2)
        sleep(1);

    for (std::thread &thread: threads) {
        thread.join();
    }

    if (sleep3)
        sleep(1);

//    sleep(10);

    system.shutdown();

    std::cout << "passed" << c_p.get_i() << "\n";
}

// no collecting, no returning
void test_production_no_collecting() {
    std::vector<std::pair<bool, bool>> how = {
            {false, false},
            {false, true},
            {true,  false},
            {true,  true}
    };

    for (std::pair<bool, bool> pa: how) {
        t_p1(10, 1, pa.first, pa.second, false);
        t_p1(1, 10, pa.first, pa.second, false);
        t_p1(5, 5, pa.first, pa.second, false);
        t_p1(14, 3, pa.first, pa.second, false);
        t_p1(3, 14, pa.first, pa.second, false);

        t_p1(10, 1, pa.first, pa.second, pa.second);
        t_p1(1, 10, pa.first, pa.second, pa.second);
        t_p1(5, 5, pa.first, pa.second, pa.first);
        t_p1(14, 3, pa.first, pa.second, pa.first);
        t_p1(3, 14, pa.first, pa.second, pa.first);
    }
}

void test_production_no_collecting2() {
    std::vector<std::string> food = {"cheeseburger", "hamburger", "milkburger", "noburger"};
    std::vector<std::string> client_order1 = {food[1], food[1], food[2], food[3]};
    std::vector<std::string> client_order2 = {food[3], food[3], food[3]};
    std::vector<std::string> client_order3 = {food[2], food[2], food[2], food[3]};
    std::vector<std::string> client_order6 = {food[1], food[1], food[2], food[1], food[0], food[2], food[3]};
    std::vector<std::string> client_order7 = {food[0]};


    std::vector<std::pair<bool, bool>> how = {
            {false, false},
            {false, true},
            {true,  false},
            {true,  true}
    };

    for (std::pair<bool, bool> pa: how) {
        t_p2({client_order1, client_order1, client_order6},
             2, false, pa.first, pa.second);
        t_p2({client_order1, client_order6, client_order6, client_order2, client_order6},
             1, false, pa.first, pa.second);
        t_p2({client_order2, client_order2, client_order6},
             10, false, pa.first, pa.second);
        t_p2({client_order2, client_order2, client_order6},
             3, false, pa.first, pa.second);
        t_p2({client_order7, client_order7, client_order7},
             3, false, pa.first, pa.second);
    }
}
