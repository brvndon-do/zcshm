#include <atomic>
#include <cstdint>
#include <cstring>
#include <new>
#include <stdexcept>

#include "zcshm/mailbox.hpp"
#include "zcshm/message_header.hpp"
#include "zcshm/shm_region.hpp"

namespace zcshm {
    Mailbox::Mailbox(ShmRegion region) : region_(std::move(region)), header_(nullptr) {}

    Mailbox Mailbox::create(const std::string &name) {
        ShmRegion region = ShmRegion::create(name, sizeof(MessageHeader));
        Mailbox mailbox{std::move(region)};

        MessageHeader* header = new(mailbox.region_.data()) MessageHeader{};
        header->ready.store(MessageHeader::kMagic, std::memory_order_release);

        mailbox.header_ = header;

        return mailbox;
    }

    Mailbox Mailbox::attach(const std::string &name) {
        ShmRegion region = ShmRegion::attach(name);

        if (region.size() < sizeof(MessageHeader))
            throw std::runtime_error("region size less than MessageHeader size");

        MessageHeader* header = reinterpret_cast<MessageHeader*>(region.data());
        if (header->ready.load(std::memory_order_acquire) != MessageHeader::kMagic)
            throw std::runtime_error("attached before initialization completed");

        Mailbox mailbox{std::move(region)};
        mailbox.header_ = header;

        return mailbox;
    }

    void Mailbox::publish(std::string_view msg) {
        if (msg.length() > MessageHeader::kCapacity)
            throw std::length_error("msg size is larger than kCapacity");

        std::uint64_t seq = header_->seq.load(std::memory_order_relaxed);

        while (seq != header_->ack.load(std::memory_order_acquire)) { } // TODO: this does not yield. fix later.

        std::memcpy(header_->payload, msg.data(), msg.length());

        header_->len = static_cast<uint32_t>(msg.length());
        header_->seq.store(seq + 1, std::memory_order_release);
    }

    bool Mailbox::tryReceive(std::string& out) {
        std::uint64_t ack = header_->ack.load(std::memory_order_relaxed);

        if (header_->seq.load(std::memory_order_acquire) == ack)
            return false;

        std::uint32_t len = header_->len;

        out.assign(reinterpret_cast<const char*>(header_->payload), len);
        header_->ack.store(ack + 1, std::memory_order_release);

        return true;
    }
}
