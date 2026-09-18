#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

#include "zcshm/mailbox.hpp"

int main() {
    zcshm::Mailbox mailbox = zcshm::Mailbox::attach("/zcshm");

    while (true) {
        std::optional<std::span<const std::byte>> span = mailbox.tryAcquire();
        if (span) {
            std::string_view sv(reinterpret_cast<const char*>(span->data()), span->size());
            std::cout << "from producer: " << sv << '\n';
            mailbox.consume();
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    return 0;
}
