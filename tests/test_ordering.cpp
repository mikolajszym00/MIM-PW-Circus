
#include "test_ordering.h"

void sleep(int sec) {
    std::this_thread::sleep_for(std::chrono::seconds(sec));
}

class Count {
public:
    Count() = default;

    int i = 0;

    int get_i() {
        i++;
        return i;
    }
};

Count c = Count();

void t_o1(int num_clients, unsigned int num_workers, bool sleep1, bool sleep2, bool sleep3) {
    System system{{}, num_workers, 1};

    std::vector<std::thread> threads;

    if (sleep1)
        sleep(1);

    for (int i = 0; i < num_clients; i++) {
        auto client = std::thread([&system]() {
            system.order({});
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

    system.shutdown();

    std::cout << "passed" << c.get_i() << "\n";
}

void test_ordering() {
    std::vector<std::pair<bool, bool>> how = {
            {false, false},
            {false, true},
            {true,  false},
            {true,  true}
    };

    for (std::pair<bool, bool> pa: how) {
        t_o1(10, 1, pa.first, pa.second, false);
        t_o1(1, 10, pa.first, pa.second, false);
        t_o1(5, 5, pa.first, pa.second, false);
        t_o1(14, 3, pa.first, pa.second, false);
        t_o1(3, 14, pa.first, pa.second, false);

        t_o1(10, 1, pa.first, pa.second, pa.second);
        t_o1(1, 10, pa.first, pa.second, pa.second);
        t_o1(5, 5, pa.first, pa.second, pa.first);
        t_o1(14, 3, pa.first, pa.second, pa.first);
        t_o1(3, 14, pa.first, pa.second, pa.first);
    }
}
