#include <iostream>
#include <string>

#include "zcshm/shm_region.hpp"

int main() {
    zcshm::ShmRegion shm = zcshm::ShmRegion::attach("/zcshm");
    std::string s = reinterpret_cast<char *>(shm.data());

    std::cout << "[output]:" << s << '\n';

    return 0;
}
