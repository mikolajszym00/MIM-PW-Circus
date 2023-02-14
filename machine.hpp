#ifndef MACHINE_HPP
#define MACHINE_HPP

#include <exception>
#include <memory>
#include <thread>
#include <utility>

class MachineFailure : public std::exception
{
};

class MachineNotWorking : public std::exception
{
};

class BadProductException : public std::exception
{
};

class Product
{
public:
//    explicit Product(std::string name) : name(std::move(name)) {};
    Product() = default;
    Product(const Product&) = delete;
    Product& operator=(const Product&) = delete;
    virtual ~Product() = default;

//    std::string name;
};

class Machine
{
public:
    virtual ~Machine() = default;
    virtual std::unique_ptr<Product> getProduct() = 0;
    virtual void returnProduct(std::unique_ptr<Product> product) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;

    int get_stan() {return stan; }

    int stan = 0;
    int sleep = 0;
};

#endif // MACHINE_HPP