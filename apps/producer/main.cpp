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
        std::string msg;
        std::cout << "message: " << std::flush;

        if (!std::getline(std::cin, msg)) {
            if (!running.load())
                break;

            // handles actual EOF (ctrl+d) or other stream errors
            std::cout << "\nstream closed or error encountered.\n";
            break;
        }

        mailbox.publish(msg);
    }

    return 0;
}
