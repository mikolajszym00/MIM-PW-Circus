#ifndef TEST_PRODUCTION_H
#define TEST_PRODUCTION_H

#include <deque>
#include <memory>
#include "../system.hpp"
#include "test_ordering.h"

template<typename T, typename V>
bool checkType(const V *v) {
    return dynamic_cast<const T *>(v) != nullptr;
}

class Burger : public Product {
};

class IceCream : public Product {
};

class Chips : public Product {
};

class CheeseBurgerMachine : public Machine {
public:
    CheeseBurgerMachine() = default;

    std::unique_ptr<Product> getProduct() override {
        return std::unique_ptr<Product>(new Burger());
    }

    void returnProduct(std::unique_ptr<Product> product) override {
        (void) product;
    }

    void start() override {}

    void stop() override {}
};

//class IceCreamMachine : public Machine {
//public:
//    std::unique_ptr<Product> getProduct() override {
//        throw MachineFailure();
//    }
//
//    void returnProduct(std::unique_ptr<Product> product) override {
//        if (!checkType<IceCream>(product.get())) throw BadProductException();
//    }
//
//    void start() override {}
//
//    void stop() override {}
//};
//
//class BurgerMachine : public Machine {
//    std::atomic_uint burgersMade;
//    std::chrono::seconds time = std::chrono::seconds(1);
//public:
//    BurgerMachine() : burgersMade(0) {}
//
//    std::unique_ptr<Product> getProduct() override {
//        if (burgersMade > 0) {
//            burgersMade--;
//            return std::unique_ptr<Product>(new Burger());
//        } else {
//            std::this_thread::sleep_for(time);
//            return std::unique_ptr<Product>(new Burger());
//        }
//    }
//
//    void returnProduct(std::unique_ptr<Product> product) override {
//        if (!checkType<Burger>(product.get())) throw BadProductException();
//        burgersMade++;
//    }
//
//    void start() override {
//        burgersMade.store(10);
//    }
//
//    void stop() override {}
//};
//
//class ChipsMachine : public Machine {
//    std::thread thread;
//    std::mutex mutex;
//    std::condition_variable cond;
//    std::atomic<int> wcount;
//    std::deque<std::unique_ptr<Chips>> queue;
//    std::atomic<bool> running;
//public:
//    ChipsMachine() : running(false) {}
//
//    std::unique_ptr<Product> getProduct() override {
//        if (!running) throw MachineNotWorking();
//        wcount++;
//        std::unique_lock<std::mutex> lock(mutex);
//        cond.wait(lock, [this]() { return !queue.empty(); });
//        wcount--;
//        auto product = std::move(queue.front());
//        queue.pop_back();
//        return product;
//    }
//
//    void returnProduct(std::unique_ptr<Product> product) override {
//        if (!checkType<Chips>(product.get())) throw BadProductException();
//        if (!running) throw MachineNotWorking();
//        std::lock_guard<std::mutex> lock(mutex);
//        queue.push_front((std::unique_ptr<Chips> &&) (std::move(product)));
//        cond.notify_one();
//    }
//
//    void start() override {
//        running = true;
//        thread = std::thread([this]() {
//            while (running || wcount > 0) {
//                int count = 7;
//                std::this_thread::sleep_for(std::chrono::seconds(1));
//                {
//                    std::lock_guard<std::mutex> lock(mutex);
//                    while (count-- > 0) {
//                        queue.push_back(std::make_unique<Chips>());
//                        cond.notify_one();
//                    }
//                }
//            }
//        });
//    }
//
//    void stop() override {
//        running = false;
//        thread.join();
//    }
//};

void test_production_no_collecting();

#endif //TEST_PRODUCTION_H
