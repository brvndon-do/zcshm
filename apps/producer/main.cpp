#include <iostream>
#include <string>

#include "zcshm/mailbox.hpp"

int main() {
    zcshm::Mailbox mailbox = zcshm::Mailbox::create("/zcshm");

    while (true) {
        std::string msg;
        std::cout << "message: ";
        std::getline(std::cin, msg);

        mailbox.publish(msg);
    }

    return 0;
}
