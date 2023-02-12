#ifndef TEST_PRODUCTION_H
#define TEST_PRODUCTION_H

#include <deque>
#include <memory>
#include <utility>
#include "../system.hpp"
#include "test_ordering.h"
#include <cassert>

template<typename T, typename V>
bool checkType(const V *v) {
    return dynamic_cast<const T *>(v) != nullptr;
}

class Burger : public Product {
public:
    explicit Burger(std::string name) : Product(std::move(name)) {};
};

class IceCream : public Product {
};

class Chips : public Product {
};

class CheeseBurgerMachine : public Machine {
public:
    CheeseBurgerMachine() = default;

    std::unique_ptr<Product> getProduct() override {
        ile++;
        stan++;
        return std::unique_ptr<Product>(new Burger( "cheeseburger" + std::to_string(ile)));
    }

    void returnProduct(std::unique_ptr<Product> product) override {
        stan--;
        (void) product;
    }

    void start() override {}

    void stop() override {}

    int ile = 0;
    int stan = 0;
};

class NoBurgerMachine : public CheeseBurgerMachine {
public:
    NoBurgerMachine() = default;

    std::unique_ptr<Product> getProduct() override {
        ile++;
        return std::unique_ptr<Product>(new Burger("noburger" + std::to_string(ile)));
    }
};

class HamBurgerMachine : public CheeseBurgerMachine {
public:
    HamBurgerMachine() = default;

    std::unique_ptr<Product> getProduct() override {
        ile++;
        return std::unique_ptr<Product>(new Burger("hamburger" + std::to_string(ile)));
    }
};

class MilkBurgerMachine : public CheeseBurgerMachine {
public:
    MilkBurgerMachine() = default;

    std::unique_ptr<Product> getProduct() override {
        ile++;
        return std::unique_ptr<Product>(new Burger("milkburger" + std::to_string(ile)));
    }
};

class IceCreamMachine : public Machine {
public:
    std::unique_ptr<Product> getProduct() override {
        std::this_thread::sleep_for(std::chrono::seconds(sleep));
        throw MachineFailure();
    }

    void returnProduct(std::unique_ptr<Product> product) override {
        stan++;
        if (!checkType<IceCream>(product.get())) throw BadProductException();
    }

    void start() override {}

    void stop() override {}
};


void test_production_no_collecting();

void test_production_no_collecting2();

void test_production_collecting();

void test_production_ordering();

void test_production_machine_broke();

void test_production_machine_broke2();

void test_production_order_expired();

#endif //TEST_PRODUCTION_H
