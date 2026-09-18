#include <atomic>
#include <csignal>
#include <iostream>
#include <ostream>
#include <string>

#include "zcshm/mailbox.hpp"

std::atomic<bool> running = true;

extern "C" void sigHandler(int sigNum) {
    if (sigNum == SIGINT)
        running.store(false);
}

int main() {
    struct sigaction sa{};
    sa.sa_handler = sigHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // no SA_RESTART
    sigaction(SIGINT, &sa, nullptr);

    zcshm::Mailbox mailbox = zcshm::Mailbox::create("/zcshm");

    // TODO: safe for now since spsc?
    while (running.load()) {
        std::cout << "message: " << std::flush;
        std::span<std::byte> span = mailbox.reserve();

        if (!std::cin.getline(reinterpret_cast<char*>(span.data()), span.size())) {
            if (!running.load())
                break;

            // handles actual EOF (ctrl+d) or other stream errors
            std::cout << "\nstream closed or error encountered.\n";
            break;
        }

        mailbox.commit(std::cin.gcount() - 1);
    }

    return 0;
}
