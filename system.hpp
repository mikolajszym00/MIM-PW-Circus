#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <exception>
#include <vector>
#include <unordered_map>
#include <functional>
#include <future>
#include <list>
#include <map>
//#include <utility>

#include <iostream>

#include "machine.hpp"

class FulfillmentFailure : public std::exception {
};

class OrderNotReadyException : public std::exception {
};

class BadOrderException : public std::exception {
};

class BadPagerException : public std::exception {
};

class OrderExpiredException : public std::exception {
};

class RestaurantClosedException : public std::exception {
};

struct WorkerReport {
    std::vector<std::vector<std::string>> collectedOrders;
    std::vector<std::vector<std::string>> abandonedOrders;
    std::vector<std::vector<std::string>> failedOrders;
    std::vector<std::string> failedProducts;
};

class System;

class CoasterPager {
public:
    CoasterPager(unsigned int id, System &system) : id(id), system(system) {
        is_ready = false;
    }

    void wait() const;

    void wait(unsigned int timeout) const;

    [[nodiscard]] unsigned int getId() const;

    [[nodiscard]] bool isReady() const;

private:
    unsigned int id;
    bool is_ready;

    System &system; // TODO: czy & ma byc
};

class System {
public:
    using product_recipient = std::mutex&;
    using machine_controller = std::mutex&;

    typedef std::unordered_map<std::string, std::shared_ptr<Machine>> machines_t;
    typedef std::unordered_map<std::string,
    std::list<std::pair<product_recipient, machine_controller>>> map_queue_t;

    System(machines_t machines, unsigned int numberOfWorkers, unsigned int clientTimeout) :
            machines(std::move(machines)),
            numberOfWorkers(numberOfWorkers),
            clientTimeout(clientTimeout) {
        system_closed = false;
        free_id = 0;
        mut_ordering_for_employees.lock();

//        mut_production_for_controller[name].lock; // TODO: kazdey zablokowac

        // TODO: stworzyc pracownikow i kontrolerow
    }


    System();

    std::vector<WorkerReport> shutdown(); // 0

    std::vector<std::string> getMenu() const; // 3

    std::vector<unsigned int> getPendingOrders() const;

    std::unique_ptr<CoasterPager> order(const std::vector<std::string> &products); // 2

    std::vector<std::unique_ptr<Product>> collectOrder(std::unique_ptr<CoasterPager> CoasterPager);

    unsigned int getClientTimeout() const;

    friend class CoasterPager;

private:
    machines_t machines;
    unsigned int numberOfWorkers;
    unsigned int clientTimeout;

    std::atomic<bool> system_closed; // TODO: jak ochronic
    unsigned int free_id;

    std::mutex mut_ordering;
    std::mutex mut_ordering_for_employees;

    std::unordered_map<std::string, std::mutex> mut_production; // TODO: jaki init
    std::unordered_map<std::string, std::mutex> mut_production_for_controller; // TODO: jaki init

    std::unordered_map<std::string, std::atomic<bool>> machine_closed;  // TODO: jak ochronic

    map_queue_t queue_to_machine;

    std::list<std::pair<unsigned int, std::vector<std::string>>> orders;
    std::map<unsigned int, std::mutex &> cp_mutex;

    void check_products(const std::vector<std::string> &products);

    std::vector<std::thread> send_threads_to_machines(
            std::unique_ptr<Product> *products,
            const std::vector<std::string> &required_machines,
            machines_t owned_machines);

    void work(const machines_t &owned_machines);

    void pick_up_product(
            std::mutex &mut_product_getter,
            std::mutex &machine_controller,
            std::unique_ptr<Product> &prod,
            const std::string &name,
            const std::shared_ptr<Machine> &machine);

    void collect_products(std::vector<std::thread> threads_to_wait_for);

    void supervise_the_machine(const std::string &name);
};

#endif // SYSTEM_HPP
