#include <atomic>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <format>
#include <signal.h>
#include <iostream>
#include <span>
#include <string>

#include "zcshm/spsc_queue.hpp"

std::atomic<bool> running = true;

extern "C" void sigHandler(int sigNum) {
    if (sigNum == SIGINT || sigNum == SIGTERM)
        running.store(false);
}

int main() {
    struct sigaction sa{};
    sa.sa_handler = sigHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // no SA_RESTART
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    try {
        zcshm::SpscQueue queue = zcshm::SpscQueue::create("/zcshm");

        // TODO: safe for now since spsc?
        while (running.load()) {
            std::cout << "message: " << std::flush;
            std::span<std::byte> span = queue.reserve();

            if (!std::cin.getline(reinterpret_cast<char*>(span.data()), span.size())) {
                if (!running.load())
                    break;

                // handles actual EOF (ctrl+d) or other stream errors
                std::cout << "\nstream closed or error encountered.\n";
                break;
            }

            queue.commit(std::cin.gcount() - 1);
        }

    } catch (const std::exception& e) {
        std::cerr << std::format("error: {}\n", e.what());
        return EXIT_FAILURE;
    }

    return 0;
}
