#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

#include "zcshm/spsc_queue.hpp"

int main() {
    zcshm::SpscQueue queue = zcshm::SpscQueue::attach("/zcshm");

    while (true) {
        std::optional<std::span<const std::byte>> span = queue.tryAcquire();

        if (!span) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            continue;
        }

        std::string_view sv(reinterpret_cast<const char*>(span->data()), span->size());
        std::cout << "from producer: " << sv << '\n';
        queue.consume();
    }

    return 0;
}
