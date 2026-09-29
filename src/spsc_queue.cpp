#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <thread>

#include "zcshm/spsc_queue.hpp"
#include "zcshm/layout.hpp"
#include "zcshm/shm_region.hpp"

namespace zcshm {
    SpscQueue::SpscQueue(ShmRegion region) : region_(std::move(region)), block_(nullptr) {}

    SpscQueue SpscQueue::create(const std::string& name) {
        ShmRegion region = ShmRegion::create(name, sizeof(ControlBlock));
        SpscQueue queue{std::move(region)};

        ControlBlock* block = new(queue.region_.data()) ControlBlock{};
        block->ready.store(ControlBlock::kMagic, std::memory_order_release);

        queue.block_ = block;

        return queue;
    }

    SpscQueue SpscQueue::attach(const std::string& name) {
        ShmRegion region = ShmRegion::attach(name);

        if (region.size() < sizeof(ControlBlock))
            throw std::runtime_error("region size less than ControlBlock size");

        ControlBlock* block = reinterpret_cast<ControlBlock*>(region.data());

        int retryCount = 0;
        while (block->ready.load(std::memory_order_acquire) != ControlBlock::kMagic && retryCount < 3) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            ++retryCount;
        }

        if (block->ready.load(std::memory_order_acquire) != ControlBlock::kMagic)
            throw std::runtime_error("region not ready");

        SpscQueue queue{std::move(region)};
        queue.block_ = block;

        return queue;
    }

    SpscQueue::SpscQueue(SpscQueue&& other) noexcept
        : region_(std::move(other.region_)), block_(other.block_) {
            other.block_ = nullptr;
    }

    SpscQueue& SpscQueue::operator=(SpscQueue&& other) noexcept {
        if (this != &other) {
            region_ = std::move(other.region_);
            block_ = other.block_;

            other.block_ = nullptr;
        }

        return *this;
    }

    void SpscQueue::publish(std::string_view msg) {
        if (msg.length() > Slot::kCapacity)
            throw std::length_error("msg size is larger than kCapacity");

        std::span<std::byte> span = reserve();
        std::memcpy(span.data(), msg.data(), msg.length());
        commit(msg.length());
    }

    std::span<std::byte> SpscQueue::reserve() {
        std::uint64_t seq = block_->seq.load(std::memory_order_relaxed); // TODO: this assumes spsc (single producer, single consumer) for now; multiple producers will break
        std::uint64_t index = seq % ControlBlock::kSlots;

        while (seq - block_->ack.load(std::memory_order_acquire) >= ControlBlock::kSlots) {
            // TODO: make this more robust
            std::this_thread::yield();
        }

        std::span<std::byte> span(block_->slots[index].payload, Slot::kCapacity);

        return span;
    }

    void SpscQueue::commit(std::size_t len) {
        if (len > Slot::kCapacity)
            throw std::length_error("len size is larger than kCapacity");

        std::uint64_t seq = block_->seq.load(std::memory_order_relaxed);
        std::uint64_t index = seq % ControlBlock::kSlots;

        block_->slots[index].len = static_cast<std::uint32_t>(len);
        block_->seq.store(seq + 1, std::memory_order_release);
    }

    bool SpscQueue::tryReceive(std::string& out) {
        std::optional<std::span<const std::byte>> span = tryAcquire();

        if (!span)
            return false;

        out.assign(reinterpret_cast<const char*>(span->data()), span->size());
        consume();

        return true;
    }

    std::optional<std::span<const std::byte>> SpscQueue::tryAcquire() {
        std::uint64_t ack = block_->ack.load(std::memory_order_relaxed); // TODO: this assumes spsc (single producer, single consumer) for now; consumers producers will break
        std::uint64_t index = ack % ControlBlock::kSlots;

        if (block_->seq.load(std::memory_order_acquire) == ack)
            return std::nullopt;

        std::uint32_t len = block_->slots[index].len;
        std::span<const std::byte> span(block_->slots[index].payload, len);

        return span;
    }

    void SpscQueue::consume() {
        std::uint64_t ack = block_->ack.load(std::memory_order_relaxed);
        block_->ack.store(ack + 1, std::memory_order_release);
    }
}
