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

using namespace std::chrono_literals;

#include <unistd.h>

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
    CoasterPager(unsigned int id, System &system) : id(id), system(system) {}

    void wait() const;

    void wait(unsigned int timeout) const;

    [[nodiscard]] unsigned int getId() const;

    [[nodiscard]] bool isReady() const;

private:
    unsigned int id;

    System &system;
};

class System {
public:
    enum class Status {
        ready, pending, expired, breakdown, collected
    };

    typedef std::unordered_map<std::string, std::shared_ptr<Machine>> machines_t;

    typedef std::vector<std::unique_ptr<Product>> unique_products_t;

    System(machines_t machines, unsigned int numberOfWorkers, unsigned int clientTimeout) :
            machines(std::move(machines)),
            numberOfWorkers(numberOfWorkers),
            clientTimeout(clientTimeout),
            system_closed(false),
            employees_joined(false),
            free_id(0),
            orders_num(0),
            reports(new WorkerReport[numberOfWorkers]),
            machine_closed_get_menu(new std::atomic<bool>[machines.size()])
            {

        unsigned int j = 0;
        for (const auto &machine: this->machines) { // create controllers
            machine_closed_get_menu[j] = false;
            machine_name_to_id[machine.first] = j;
            machine.second->start();

            const std::string &name = machine.first;
            std::thread t{[this, name] { supervise_the_machine(name); }};

            threads_controllers.push_back(std::move(t));
            j++;
        }

        for (unsigned int i = 0; i < this->numberOfWorkers; i++) { // create employees
            std::thread t{[this, i] { work(this->machines, i, reports[i]); }};

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
    machines_t machines;
    unsigned int numberOfWorkers;
    unsigned int clientTimeout;

    std::vector<std::thread> threads_controllers;
    std::vector<std::thread> threads_employees;

    std::atomic<bool> system_closed;
    std::atomic<bool> employees_joined;
    unsigned int free_id;

    // order
    std::mutex mut_ordering;
    std::mutex mut_ordering_for_employees;
    std::condition_variable cv_ordering_for_employees;
    unsigned int orders_num;
    std::list<std::pair<unsigned int, std::vector<std::string>>> orders;

    // production
    ConcurrentUnorderedMap<std::string, std::mutex> mut_production_for_controller;
    ConcurrentUnorderedMap<std::string, std::condition_variable> cv_production_for_controller;
    ConcurrentUnorderedMap<std::string, unsigned int> queue_size;

    // communication assistant-controller
    ConcurrentUnorderedMap<std::string, std::mutex> mut_assistant;
    ConcurrentUnorderedMap<std::string, std::condition_variable> cv_assistant;
    ConcurrentUnorderedMap<std::string, bool> bool_assistant;
    ConcurrentUnorderedMap<std::string, std::mutex> mut_controller;
    ConcurrentUnorderedMap<std::string, std::condition_variable> cv_controller;
    ConcurrentUnorderedMap<std::string, bool> bool_controller;

    // coaster pager
    ConcurrentUnorderedMap<unsigned int, std::mutex> mut_coaster_pager;
    ConcurrentUnorderedMap<unsigned int, std::condition_variable> cv_coaster_pager;
    ConcurrentUnorderedMap<unsigned int, bool> bool_coaster_pager;
    std::map<unsigned int, bool> is_ready;

    // collect
    ConcurrentUnorderedMap<unsigned int, unique_products_t> completed_meals;

    // reports
    WorkerReport* reports;

    // sleep
    ConcurrentUnorderedMap<unsigned int, std::mutex> mut_sleep;
    ConcurrentUnorderedMap<unsigned int, std::condition_variable> cv_sleep;
    ConcurrentUnorderedMap<unsigned int, bool> bool_sleep;


    ConcurrentUnorderedMap<std::string, std::atomic<bool>> machine_closed;
    ConcurrentUnorderedMap<std::string, unsigned int> machine_name_to_id;
    std::atomic<bool>* machine_closed_get_menu;

    ConcurrentUnorderedMap<unsigned int, Status> orders_status;

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
                                machines_t &owned_machines,
                                WorkerReport &report);

    void prepare_products_for_picking_up(
            unsigned int id_employee,
            unsigned int id,
            unique_products_t products,
            const std::vector<std::string> &required_machines,
            machines_t &owned_machines,
            WorkerReport &report);

    void prepare_products_for_returning(
            unsigned int id_employee,
            unsigned int id,
            unique_products_t products,
            const std::vector<std::string> &required_machines,
            machines_t &owned_machines,
            WorkerReport &report);

    void work(machines_t &owned_machines, unsigned int id_employee, WorkerReport &report);

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
