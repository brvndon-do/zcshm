#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "zcshm/mailbox.hpp"

int main() {
    zcshm::Mailbox mailbox = zcshm::Mailbox::attach("/zcshm");
    std::string out;

    while (true) {
        if (mailbox.tryReceive(out)) {
            std::cout << "from producer: " << out << '\n';
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    return 0;
}
