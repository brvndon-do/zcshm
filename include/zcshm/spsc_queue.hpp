#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "layout.hpp"
#include "shm_region.hpp"

namespace zcshm {
    class SpscQueue {
    private:
        ShmRegion region_;
        ControlBlock* block_;

        SpscQueue(ShmRegion region);
    public:
        static SpscQueue create(const std::string& name);
        static SpscQueue attach(const std::string& name);

        // move
        SpscQueue(SpscQueue&& other) noexcept;
        SpscQueue& operator=(SpscQueue&& other) noexcept;

        // copy (do not generate)
        SpscQueue(const SpscQueue&)=delete;
        SpscQueue& operator=(const SpscQueue&)=delete;

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
