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

    [[nodiscard]] bool isReady() const; // TODO jest gotowe zamowienie ale czy poprawnie

private:
    unsigned int id;
    bool is_ready;

    System &system; // TODO: czy & ma byc
};

class System {
public:
    enum class Status { ready, pending, expired, breakdown }; // jeśli nie ma w bazie to oznacza ze odebrane

    using product_recipient = std::mutex &;
    using machine_controller = std::mutex &;

    typedef std::unordered_map<std::string, std::shared_ptr<Machine>> machines_t;
    typedef std::unordered_map<std::string,
            std::list<std::pair<product_recipient, machine_controller>>> map_queue_t;

    System(machines_t machines, unsigned int numberOfWorkers, unsigned int clientTimeout) :
            machines(std::move(machines)),
            numberOfWorkers(numberOfWorkers),
            clientTimeout(clientTimeout) {
        for (const auto& machine: machines) { // create controllers
            mut_production_for_controller[machine.first].lock();

            const std::string& name = machine.first;
            std::thread t{[this, name] { supervise_the_machine(name); }};

            threads_controllers.push_back(std::move(t));
        }

        mut_ordering_for_employees.lock();
        for (unsigned int i = 0; i < numberOfWorkers; i++) { // create employees
            std::thread t{[this, machines] { work(machines); }};

            threads_employees.push_back(std::move(t));
        }

        system_closed = false;
        free_id = 0;
    }


    System() = default;

    std::vector<WorkerReport> shutdown();

    std::vector<std::string> getMenu() const;

    std::vector<unsigned int> getPendingOrders() const;

    std::unique_ptr<CoasterPager> order(const std::vector<std::string> &products);

    std::vector<std::unique_ptr<Product>> collectOrder(std::unique_ptr<CoasterPager> CoasterPager);

    unsigned int getClientTimeout() const;

    friend class CoasterPager;

private:
    machines_t machines;
    unsigned int numberOfWorkers;
    unsigned int clientTimeout;

    std::vector<std::thread> threads_controllers;
    std::vector<std::thread> threads_employees;

    std::atomic<bool> system_closed; // TODO: jak ochronic
    unsigned int free_id;

    std::mutex mut_ordering;
    std::mutex mut_ordering_for_employees;

    std::unordered_map<std::string, std::mutex> mut_production;
    std::unordered_map<std::string, std::mutex> mut_production_for_controller;

    std::mutex mut_completed_meals;

    std::unordered_map<std::string, std::atomic<bool>> machine_closed;  // TODO: jak ochronic

    map_queue_t queue_to_machine;

    std::list<std::pair<unsigned int, std::vector<std::string>>> orders;
    std::unordered_map<unsigned int, Status> orders_status;

    std::unordered_map<unsigned int, std::mutex &> mut_coaster_pager;
    std::unordered_map<unsigned int, std::vector<std::unique_ptr<Product>>> completed_meals;

    void check_products(const std::vector<std::string> &products);

    std::vector<std::thread> send_threads_to_machines(
            std::vector<std::unique_ptr<Product>> products,
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
