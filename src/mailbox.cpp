#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <new>
#include <stdexcept>
#include <thread>

#include "zcshm/mailbox.hpp"
#include "zcshm/layout.hpp"
#include "zcshm/shm_region.hpp"

namespace zcshm {
    Mailbox::Mailbox(ShmRegion region) : region_(std::move(region)), block_(nullptr) {}

    Mailbox Mailbox::create(const std::string &name) {
        ShmRegion region = ShmRegion::create(name, sizeof(ControlBlock));
        Mailbox mailbox{std::move(region)};

        ControlBlock* block = new(mailbox.region_.data()) ControlBlock{};
        block->ready.store(ControlBlock::kMagic, std::memory_order_release);

        mailbox.block_ = block;

        return mailbox;
    }

    Mailbox Mailbox::attach(const std::string &name) {
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

        Mailbox mailbox{std::move(region)};
        mailbox.block_ = block;

        return mailbox;
    }

    void Mailbox::publish(std::string_view msg) {
        if (msg.length() > Slot::kCapacity)
            throw std::length_error("msg size is larger than kCapacity");

        std::uint64_t seq = block_->seq.load(std::memory_order_relaxed); // TODO: this assumes spsc (single producer, single consumer) for now; multiple producers will break
        std::uint64_t index = seq % ControlBlock::kSlots;

        while (seq - block_->ack.load(std::memory_order_acquire) >= ControlBlock::kSlots) {
            // TODO: make this more robust
            std::this_thread::yield();
        }

        std::memcpy(block_->slots[index].payload, msg.data(), msg.length());

        block_->slots[index].len = static_cast<std::uint32_t>(msg.length());
        block_->seq.store(seq + 1, std::memory_order_release);
    }

    bool Mailbox::tryReceive(std::string& out) {
        std::uint64_t ack = block_->ack.load(std::memory_order_relaxed); // TODO: this assumes spsc (single producer, single consumer) for now; consumers producers will break
        std::uint64_t index = ack % ControlBlock::kSlots;

        if (block_->seq.load(std::memory_order_acquire) == ack)
            return false;

        std::uint32_t len = block_->slots[index].len;

        out.assign(reinterpret_cast<const char*>(block_->slots[index].payload), len);
        block_->ack.store(ack + 1, std::memory_order_release);

        return true;
    }
}
