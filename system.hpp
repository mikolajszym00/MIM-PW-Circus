#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <exception>
#include <vector>
#include <unordered_map>
#include <functional>
#include <future>

#include "machine.hpp"

class FulfillmentFailure : public std::exception
{
};

class OrderNotReadyException : public std::exception
{
};

class BadOrderException : public std::exception
{
};

class BadPagerException : public std::exception
{
};

class OrderExpiredException : public std::exception
{
};

class RestaurantClosedException : public std::exception
{
};

struct WorkerReport
{
    std::vector<std::vector<std::string>> collectedOrders;
    std::vector<std::vector<std::string>> abandonedOrders;
    std::vector<std::vector<std::string>> failedOrders;
    std::vector<std::string> failedProducts;
};

class CoasterPager
{
public:
    void wait() const;

    void wait(unsigned int timeout) const;

    [[nodiscard]] unsigned int getId() const;

    [[nodiscard]] bool isReady() const;
};

class System
{
public:
    typedef std::unordered_map<std::string, std::shared_ptr<Machine>> machines_t;
    
    System(machines_t machines, unsigned int numberOfWorkers, unsigned int clientTimeout) :
        machines(std::move(machines)),
        numberOfWorkers(numberOfWorkers),
        clientTimeout(clientTimeout)
        {
            closed = false;
        }


    System();

    std::vector<WorkerReport> shutdown(); // 0

    std::vector<std::string> getMenu() const; // 3

    std::vector<unsigned int> getPendingOrders() const;

    std::unique_ptr<CoasterPager> order(std::vector<std::string> products);

    std::vector<std::unique_ptr<Product>> collectOrder(std::unique_ptr<CoasterPager> CoasterPager);

    unsigned int getClientTimeout() const;


private:
    machines_t machines;
    unsigned int numberOfWorkers;
    unsigned int clientTimeout;

    bool closed;

    void check_products(const std::vector<std::string> &products);
};

#endif // SYSTEM_HPP

// 1 - zrobione pobieznie
// 2 - zrobione lepiej
// 3 - przemyslane wszsykie warianty
// 4 - z testami
// 5 - ostateczne



