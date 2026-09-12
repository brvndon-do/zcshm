#include <iostream>
#include <string>

#include "zcshm/mailbox.hpp"

int main() {
    zcshm::Mailbox mailbox = zcshm::Mailbox::attach("/zcshm");
    std::string out;
    while (mailbox.tryReceive(out)) {
        std::cout << "from producer: " << out << '\n';
    }

    return 0;
}
