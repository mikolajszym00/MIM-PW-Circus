#ifndef TEST_PRODUCTION_H
#define TEST_PRODUCTION_H

#include <deque>
#include <memory>
#include <utility>
#include "../system.hpp"
#include "test_ordering.h"

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
        return std::unique_ptr<Product>(new Burger( "cheeseburger" + std::to_string(ile)));
    }

    void returnProduct(std::unique_ptr<Product> product) override {
        (void) product;
    }

    void start() override {}

    void stop() override {}

    int ile = 0;
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

void test_production_no_collecting();

void test_production_no_collecting2();

void test_production_collecting();

void test_production_ordering();

#endif //TEST_PRODUCTION_H
