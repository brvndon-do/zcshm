#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "zcshm/shm_region.hpp"

int main() {
    zcshm::ShmRegion shm = zcshm::ShmRegion::create("/zcshm", 4096);
    std::string msg = "Hello, from producer!";
    std::memcpy(shm.data(), msg.c_str(), msg.length() + 1);

    for (;;) {
        std::cout << "awaiting...\n";
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    return 0;
}
