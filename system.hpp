#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <exception>
#include <vector>
#include <unordered_map>
#include <functional>
#include <future>
#include <list>
#include <map>
#include <deque>
//#include <utility>

#include <iostream>

#include "machine.hpp"
#include "concurrent_map.h"

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
    enum class Status {
        ready, pending, expired, breakdown
    }; // jeśli nie ma w bazie to oznacza ze odebrane

    using mut_secrity = std::shared_ptr<std::mutex>;
    using cv_secrity = std::shared_ptr<std::condition_variable>;

    typedef std::unordered_map<std::string, std::shared_ptr<Machine>> machines_t;
//    typedef std::unordered_map<std::string, std::shared_ptr<Machine>> machines_t_stare;

    typedef std::pair<std::pair<mut_secrity, cv_secrity>, bool> security;

    typedef ConcurrentUnorderedMap<std::string, std::deque<std::pair<security, security>>> map_queue_t;

    typedef std::vector<std::unique_ptr<Product>> unique_products_t;

    System(machines_t machines, unsigned int numberOfWorkers, unsigned int clientTimeout) :
            machines(std::move(machines)),
            numberOfWorkers(numberOfWorkers),
            clientTimeout(clientTimeout),
            system_closed(false),
            employees_joined(false),
            free_id(0),
            orders_num(0) {

//        for (const auto &machine: machines) {
//            machines[machine.first] = machine.second;
//        }

        for (const auto &machine: this->machines) { // create controllers
            machine.second->start();

            const std::string &name = machine.first;
            std::thread t{[this, name] { supervise_the_machine(name); }};

            threads_controllers.push_back(std::move(t));
        }

        for (unsigned int i = 0; i < numberOfWorkers; i++) { // create employees
            std::thread t{[this, i] { work(this->machines, i); }};

            threads_employees.push_back(std::move(t));
        }
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
    std::mutex mut_print;

    machines_t machines; // TODO czy to nie powinna byc mapa
    unsigned int numberOfWorkers;
    unsigned int clientTimeout;

    std::vector<std::thread> threads_controllers;
    std::vector<std::thread> threads_employees;

    std::atomic<bool> system_closed; // TODO: jak ochronic
    std::atomic<bool> employees_joined; // TODO: jak ochronic
    unsigned int free_id;

    // order
    std::mutex mut_ordering;
    std::mutex mut_ordering_for_employees;
    std::condition_variable cv_ordering_for_employees;
    unsigned int orders_num;

    // production
    ConcurrentUnorderedMap<std::string, std::mutex> mut_production;
    ConcurrentUnorderedMap<std::string, std::mutex> mut_production_for_controller;
    ConcurrentUnorderedMap<std::string, std::condition_variable> cv_production_for_controller;
    ConcurrentUnorderedMap<std::string, unsigned int> queue_size;

    // communication recipient-controller
    ConcurrentUnorderedMap<std::string, std::mutex> mut_recipient;
    ConcurrentUnorderedMap<std::string, std::condition_variable> cv_recipient;
    ConcurrentUnorderedMap<std::string, bool> bool_recipient;
    ConcurrentUnorderedMap<std::string, std::mutex> mut_controller;
    ConcurrentUnorderedMap<std::string, std::condition_variable> cv_controller;
    ConcurrentUnorderedMap<std::string, bool> bool_controller;

    // coaster pager
    ConcurrentUnorderedMap<unsigned int, std::mutex> mut_coaster_pager;
    ConcurrentUnorderedMap<unsigned int, std::condition_variable> cv_coaster_pager;
    ConcurrentUnorderedMap<unsigned int, bool> bool_coaster_pager;

    std::mutex mut_completed_meals;

    ConcurrentUnorderedMap<std::string, std::atomic<bool>> machine_closed;
    map_queue_t queue_to_machine;

    std::list<std::pair<unsigned int, std::vector<std::string>>> orders; // ma mutex, oczyszczana
    ConcurrentUnorderedMap<unsigned int, Status> orders_status;


    ConcurrentUnorderedMap<unsigned int, unique_products_t> completed_meals;

    void check_products(const std::vector<std::string> &products);

    std::vector<std::thread>
    send_threads_to_machines(unsigned int id_employee,
                             std::vector<std::future<std::unique_ptr<Product>>> &futures,
                             std::vector<std::promise<std::unique_ptr<Product>>> &promises,
                             const std::vector<std::string> &required_machines,
                             machines_t &owned_machines);

    std::vector<std::thread>
    return_products_to_machines(unsigned int id_employee,
                                unique_products_t products,
                                const std::vector<std::string> &required_machines,
                                machines_t &owned_machines);

    void work(machines_t &owned_machines, unsigned int id_employee);

    void pick_up_product(unsigned int id_employee,
                         std::promise<std::unique_ptr<Product>> &promise,
                         const std::string &name,
                         std::shared_ptr<Machine> &machine);

    void return_product(unsigned int id_employee,
                        std::unique_ptr<Product> product,
                        const std::string &name,
                        std::shared_ptr<Machine> &machine);

    unique_products_t collect_products(std::vector<std::thread> threads_to_wait_for,
                                       std::vector<std::future<std::unique_ptr<Product>>> futures);

    void wait_for_return(std::vector<std::thread> threads_to_wait_for);

    void supervise_the_machine(const std::string &name);

    void clean_after_order(unsigned int id);

};

#endif // SYSTEM_HPP
