
#include "test_production.h"
//
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

static std::vector<std::string> food = {"cheeseburger", "hamburger", "icecream", "noburger"};
static std::vector<std::string> client_order0 = {};
static std::vector<std::string> client_order1 = {food[1], food[1], food[2], food[3]};
static std::vector<std::string> client_order2 = {food[3], food[3], food[3]};
static std::vector<std::string> client_order3 = {food[2], food[2], food[2], food[3]};
static std::vector<std::string> client_order6 = {food[1], food[1], food[2], food[1], food[0], food[2], food[3]};
static std::vector<std::string> client_order7 = {food[0]};
static std::vector<std::string> client_order8 = {food[0], food[0], food[0], food[0], food[0], food[0], food[0]};
static std::vector<std::string> client_order_one_broken = {food[2],
                                                           food[1], food[1], food[1], food[0], food[0], food[0],
                                                           food[0],
                                                           food[1], food[1], food[1], food[0], food[0], food[0],
                                                           food[2]};
static std::vector<std::string> client_order_big = {food[1], food[3], food[3], food[0], food[0], food[3], food[0],
                                                    food[3], food[1], food[1], food[0], food[3], food[0], food[3]};


static std::vector<std::pair<bool, bool>> where_sleep = {
        {false, false},
        {false, true},
        {true,  false},
        {true,  true}
};

void cmp_order_products(const std::vector<std::string> &order, const std::vector<std::unique_ptr<Product>> &prod) {
    assert(order.size() == prod.size());
//
//    for(unsigned int i = 0; i < order.size(); i++) {
//        prod[i].substr()
//    }
}

void
print_cmp_order_products(const std::vector<std::string> &order, const std::vector<std::unique_ptr<Product>> &prod) {
    std::cout << "Zamowienie: ";
    for (const std::string &o: order) {
        std::cout << o << " ";
    }
    std::cout << "\nRzeczywistosc: ";
    for (const std::unique_ptr<Product> &p: prod) {
        std::cout << p->name << " ";
    }

    std::cout << "\n\n";
}

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

void
t_p2(const std::vector<std::vector<std::string>> &clients_orders, unsigned int num_workers, bool sleep1, bool sleep2,
     bool sleep3) {
    System system{
            {
                    {"cheeseburger", std::shared_ptr<Machine>(new CheeseBurgerMachine())},
                    {"hamburger", std::shared_ptr<Machine>(new CheeseBurgerMachine())},
                    {"milkburger", std::shared_ptr<Machine>(new CheeseBurgerMachine())},
                    {"noburger", std::shared_ptr<Machine>(new CheeseBurgerMachine())}
            },
            num_workers, 1};

    std::vector<std::thread> threads;


    for (const std::vector<std::string> &client_order: clients_orders) {
        if (sleep1)
            usleep(10000);

        auto client = std::thread([&system, client_order]() {
            system.order(client_order);
        });

        threads.push_back(std::move(client));
    }

    if (sleep2)
        usleep(10000);

    for (std::thread &thread: threads) {
        thread.join();
    }

    if (sleep3)
        sleep(1);

//    sleep(10);

    system.shutdown();

    std::cout << "passed" << c_p.get_i() << "\n";
}

void
t_p3(const std::vector<std::vector<std::string>> &clients_orders,
     unsigned int num_workers,
     bool sleep1,
     bool sleep2,
     bool sleep3,
     bool collect) {

    typedef std::pair<std::vector<std::string>, std::vector<std::unique_ptr<Product>>> giga_para;
    ConcurrentUnorderedMap<unsigned int, giga_para> sol;

    System system{
            {
                    {"cheeseburger", std::shared_ptr<Machine>(new CheeseBurgerMachine())},
                    {"hamburger", std::shared_ptr<Machine>(new HamBurgerMachine())},
                    {"milkburger", std::shared_ptr<Machine>(new MilkBurgerMachine())},
                    {"noburger", std::shared_ptr<Machine>(new NoBurgerMachine())}
            },
            num_workers, 1000}; // 1 sek


    if (sleep1)
        sleep(1);

    std::vector<std::thread> threads;
    std::vector<std::future<giga_para>> futures;
    std::vector<std::future<unsigned int>> futures_id;
    std::vector<std::promise<unsigned int>> promises_id;
    std::vector<std::promise<giga_para>> promises;

    for (unsigned int i = 0; i < clients_orders.size(); i++) {
        std::promise<giga_para> pms;
        std::future<giga_para> ftr = pms.get_future();

        promises.push_back(std::move(pms));
        futures.push_back(std::move(ftr));

        std::promise<unsigned int> pms1;
        std::future<unsigned int> ftr1 = pms1.get_future();

        promises_id.push_back(std::move(pms1));
        futures_id.push_back(std::move(ftr1));
    }

    unsigned int i = 0;
    for (const std::vector<std::string> &client_order: clients_orders) {
        auto client = std::thread([i, &system, client_order, collect, &promises, &promises_id]() {
            auto p = system.order(client_order);

            if (collect) {
                p->wait();
                unsigned int num = p->getId();
                promises_id[i].set_value(num);

                std::vector<std::unique_ptr<Product>> prod = system.collectOrder(std::move(p));
                promises[i].set_value(std::make_pair(client_order, std::move(prod)));
            }
        });

        threads.push_back(std::move(client));
        i++;
    }
    if (sleep2)
        sleep(1);

    i = 0;
    for (std::thread &thread: threads) {
        unsigned int id = futures_id[i].get();
        sol[id] = futures[i].get();
        thread.join();
        i++;
    }

    if (sleep3)
        sleep(1);

//    sleep(10);

    system.shutdown();

    // wypisanie
//    for (unsigned int j = 0; j < i; j++) {
//        std::cout << "joty " << j << "\n";
//        print_cmp_order_products(sol[j].first, sol[j].second);
//    }

    std::cout << "passed" << c_p.get_i() << "\n";
}


std::vector<WorkerReport>
t_p4(System &system,
     const std::vector<std::vector<std::string>> &clients_orders,
     bool sleep1,
     bool sleep2,
     bool sleep3,
     bool collect,
     bool expired) {

    typedef std::pair<std::vector<std::string>, std::vector<std::unique_ptr<Product>>> giga_para;
    ConcurrentUnorderedMap<unsigned int, giga_para> sol;

    if (sleep1)
        sleep(1);

    std::vector<std::thread> threads;
    std::vector<std::future<giga_para>> futures;
    std::vector<std::future<unsigned int>> futures_id;
    std::vector<std::promise<unsigned int>> promises_id;
    std::vector<std::promise<giga_para>> promises;

    for (unsigned int i = 0; i < clients_orders.size(); i++) {
        std::promise<giga_para> pms;
        std::future<giga_para> ftr = pms.get_future();

        promises.push_back(std::move(pms));
        futures.push_back(std::move(ftr));

        std::promise<unsigned int> pms1;
        std::future<unsigned int> ftr1 = pms1.get_future();

        promises_id.push_back(std::move(pms1));
        futures_id.push_back(std::move(ftr1));
    }

    unsigned int i = 0;
    std::map<unsigned int, bool> right_i;

    for (const std::vector<std::string> &client_order: clients_orders) {
        right_i.emplace(i, true);

        auto client = std::thread([&right_i, i, &system, client_order, collect, expired, &promises, &promises_id]() {
//        auto client = std::thread([&system, client_order, collect]() {
            try {
                auto p = system.order(client_order);

                if (collect) {
//                    if (i % 2 == 0) {
                    p->wait();
//                    }

                    unsigned int num = p->getId();

                    if (expired) {
                        sleep(1);
                    }

                    std::vector<std::unique_ptr<Product>> prod;
//                    if (i % 2 == 0) {
                    prod = system.collectOrder(std::move(p));
//                    } else {
//                        prod.emplace_back(nullptr);
//                    }

                    promises_id[i].set_value(num);
                    promises[i].set_value(std::make_pair(client_order, std::move(prod)));
                }
            }
            catch (const RestaurantClosedException &e) {
                right_i[i] = false;
                promises_id[i].set_value(10000);
            }
            catch (const BadOrderException &e) {
                right_i[i] = false;
                promises_id[i].set_value(10000);
            }
            catch (const FulfillmentFailure &e) {
                right_i[i] = false;
                promises_id[i].set_value(10001);
            }
            catch (const OrderExpiredException &e) {
                right_i[i] = false;
                promises_id[i].set_value(10001);
            }
            catch (const OrderNotReadyException &e) {
                right_i[i] = false;
                promises_id[i].set_value(10001);
            }

        });
        threads.push_back(std::move(client));

        i++;
    }
    if (sleep2)
        sleep(1);

    i = 0;
    for (std::thread &thread: threads) {
        unsigned int id = futures_id[i].get();
        if (right_i[i]) {
            sol[id] = futures[i].get();
        }
        thread.join();
        i++;
    }

    if (sleep3)
        sleep(1);

//    sleep(10);

    return system.shutdown();

//     wypisanie
//    for (unsigned int j = 0; j < i; j++) {
//        std::cout << "joty " << j << "\n";
//        print_cmp_order_products(sol[j].first, sol[j].second);
//    }

//    std::cout << "passed" << c_p.get_i() << "\n";
}

// no collecting, no returning
void test_production_no_collecting() {
    for (std::pair<bool, bool> pa: where_sleep) {
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
    for (std::pair<bool, bool> pa: where_sleep) {
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

void test_production_collecting() {
    for (std::pair<bool, bool> pa: where_sleep) {
        t_p3({client_order1, client_order1, client_order6},
             2, false, pa.first, pa.second, true);
        t_p3({client_order1, client_order6, client_order6, client_order2, client_order6},
             1, false, pa.first, pa.second, true);
        t_p3({client_order2, client_order2, client_order6},
             10, false, pa.first, pa.second, true);
        t_p3({client_order2, client_order2, client_order6},
             3, false, pa.first, pa.second, true);
        t_p3({client_order7, client_order7, client_order7},
             3, false, pa.first, pa.second, true);
    }
}

void test_production_ordering() {
    t_p3({client_order8, client_order8, client_order8, client_order8,
          client_order8, client_order8, client_order8, client_order8},
         4, false, false, false, true);

    auto mach_cheese = std::shared_ptr<Machine>(new CheeseBurgerMachine());
    auto mach_ham = std::shared_ptr<Machine>(new HamBurgerMachine());
    auto mach_no = std::shared_ptr<Machine>(new NoBurgerMachine());

    mach_no->sleep = 1;

    System system_broken_machine{{
                                         {"cheeseburger", mach_cheese},
                                         {"hamburger", mach_ham},
                                         {"noburger", mach_no}
                                 }, 3, 1
    };

    t_p4(system_broken_machine, {client_order_big, client_order_big, client_order_big, client_order_big,
                                 client_order_big, client_order_big, client_order_big},
         false, false, false, true, false);
}

void test_production_machine_broke() {
    for (std::pair<bool, bool> pa: where_sleep) {
        auto mach_ice = std::shared_ptr<Machine>(new IceCreamMachine());
        auto mach_cheese = std::shared_ptr<Machine>(new CheeseBurgerMachine());
        auto mach_ham = std::shared_ptr<Machine>(new HamBurgerMachine());
        auto mach_no = std::shared_ptr<Machine>(new NoBurgerMachine());

        System system_broken_machine{{
                                             {"cheeseburger", mach_cheese},
                                             {"hamburger", mach_ham},
                                             {"icecream", mach_ice},
                                             {"noburger", mach_no}
                                     }, 5, 1
        };

        t_p4(system_broken_machine, {client_order3, client_order1, client_order3, client_order2,
                                     client_order1, client_order1, client_order8, client_order3},
             pa.first, pa.first, pa.second, true, false);

        assert(mach_cheese->stan == 0);
        assert(mach_ham->stan == 0);
        assert(mach_no->stan == 0);
    }
}

// dużo przyjętych zamówień do zwrotu
void test_production_machine_broke2() { // wywalilo//
    auto mach_ice = std::shared_ptr<Machine>(new IceCreamMachine());
    mach_ice->sleep = 1;

    auto mach_cheese = std::shared_ptr<Machine>(new CheeseBurgerMachine());
    auto mach_ham = std::shared_ptr<Machine>(new HamBurgerMachine());
    auto mach_no = std::shared_ptr<Machine>(new NoBurgerMachine());

    System system_broken_machine{{
                                         {"cheeseburger", mach_cheese},
                                         {"hamburger", mach_ham},
                                         {"icecream", mach_ice},
                                         {"noburger", mach_no}
                                 }, 1, 1
    };

    t_p4(system_broken_machine, {client_order_one_broken, client_order3, client_order6},
         false, false, false, true, false);

    assert(mach_cheese->stan == 0);
    assert(mach_ham->stan == 0);
    assert(mach_no->stan == 0);

    System system_broken_machine2{{
                                          {"cheeseburger", mach_cheese},
                                          {"hamburger", mach_ham},
                                          {"icecream", mach_ice},
                                          {"noburger", mach_no}
                                  }, 4, 1
    };

    t_p4(system_broken_machine2, {client_order_one_broken,
                                  client_order3, client_order3, client_order3, client_order6,
                                  client_order7, client_order8, client_order_one_broken},
         false, false, false, true, false);

    assert(mach_cheese->stan == 0);
    assert(mach_ham->stan == 0);
    assert(mach_no->stan == 0);

    mach_cheese->sleep = 1;

    System system_broken_machine3{{
                                          {"cheeseburger", mach_cheese},
                                          {"hamburger", mach_ham},
                                          {"icecream", mach_ice},
                                          {"noburger", mach_no}
                                  }, 1, 1
    };

    t_p4(system_broken_machine3, {client_order_one_broken, client_order3, client_order3, client_order3, client_order6,
                                  client_order7, client_order8, client_order_one_broken},
         false, false, false, true, false);

    assert(mach_cheese->stan == 0);
    assert(mach_ham->stan == 0);
    assert(mach_no->stan == 0);
}


void test_production_order_expired() {
    auto mach_cheese = std::shared_ptr<Machine>(new CheeseBurgerMachine());
    auto mach_ham = std::shared_ptr<Machine>(new HamBurgerMachine());
    auto mach_no = std::shared_ptr<Machine>(new NoBurgerMachine());

    mach_no->sleep = 1;

    System system_broken_machine{{
                                         {"cheeseburger", mach_cheese},
                                         {"hamburger", mach_ham},
                                         {"noburger", mach_no}
                                 }, 3, 10000000
    };

    t_p4(system_broken_machine, {client_order_big, client_order_big, client_order_big, client_order_big,
                                 client_order_big, client_order_big, client_order_big},
         false, false, false, true, false);

    System system_b{{
                                         {"cheeseburger", mach_cheese},
                                         {"hamburger", mach_ham},
                                         {"noburger", mach_no}
                                 }, 3, 1
    };

    t_p4(system_b, {client_order_big, client_order_big, client_order_big, client_order_big,
                                 client_order_big, client_order_big, client_order_big},
         false, false, false, true, true);
}

void test_production_report() {
    auto mach_ice = std::shared_ptr<Machine>(new IceCreamMachine());
    mach_ice->sleep = 1;
    auto mach_cheese = std::shared_ptr<Machine>(new CheeseBurgerMachine());
    auto mach_ham = std::shared_ptr<Machine>(new HamBurgerMachine());
    auto mach_no = std::shared_ptr<Machine>(new NoBurgerMachine());

    mach_no->sleep = 1;

    System system_machine2{{
                                   {"cheeseburger", mach_cheese},
                                   {"hamburger", mach_ham},
                                   {"noburger", mach_no},
                                   {"icecream", mach_ice}
                           }, 3, 1
    };

    std::vector<WorkerReport> wr = t_p4(system_machine2, {client_order_big, client_order_big, client_order_big,
                                                          client_order1, client_order0, client_order_big,
                                                          client_order_big, client_order_big, client_order_big},
                                        false, false, false, true, false);

    for (const WorkerReport &w : wr) {
        std::cout << "WR: " << "\n\n";

        std::cout << "collectedOrders: " << "\n\n";
        int i = 0;
        for (const std::vector<std::string> &coll_vec: w.collectedOrders) {
            std::cout << "Zamowinie: " << i << '\n';
            for (const std::string &coll: coll_vec) {
                std::cout << coll << " ";
            }
            i++;
            std::cout << '\n';
        }

        std::cout << "failedOrders: " << "\n\n";
        i = 0;
        for (const std::vector<std::string> &coll_vec: w.failedOrders) {
            std::cout << "Zamowinie: " << i << '\n';
            for (const std::string &coll: coll_vec) {
                std::cout << coll << " ";
            }
            i++;
            std::cout << '\n';
        }

        std::cout << "abandonedOrders: " << "\n\n";
        i = 0;
        for (const std::vector<std::string> &coll_vec: w.abandonedOrders) {
            std::cout << "Zamowinie: " << i << '\n';
            for (const std::string &coll: coll_vec) {
                std::cout << coll << " ";
            }
            i++;
            std::cout << '\n';
        }

        std::cout << "failedProducts: " << "\n\n";
        i = 0;
        for (const std::string &coll_vec: w.failedProducts) {
            std::cout << coll_vec << " ";

            i++;
            std::cout << '\n';
        }
    }
}