#pragma once

#include <cstddef>
#include <optional>
#include <span>
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

        // move
        Mailbox(Mailbox&& other) noexcept;
        Mailbox& operator=(Mailbox&& other) noexcept;

        // copy (do not generate)
        Mailbox(const Mailbox&)=delete;
        Mailbox& operator=(const Mailbox&)=delete;

        // producer
        void publish(std::string_view msg);
        std::span<std::byte> reserve();
        void commit(std::size_t len);

        // consumer
        bool tryReceive(std::string& out);
        std::optional<std::span<const std::byte>> tryAcquire();
        void consume();
    };
}
