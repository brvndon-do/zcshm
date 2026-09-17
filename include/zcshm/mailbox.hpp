#pragma once

#include <string>
#include <string_view>

#include "layout.hpp"
#include "shm_region.hpp"

namespace zcshm {
    class Mailbox {
    private:
        ShmRegion region_;
        ControlBlock* block_;

        Mailbox(ShmRegion region);
    public:
        static Mailbox create(const std::string& name);
        static Mailbox attach(const std::string& name);

        void publish(std::string_view msg);
        bool tryReceive(std::string& out);
    };
}
