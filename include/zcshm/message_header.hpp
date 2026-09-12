#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace zcshm {
    struct MessageHeader {
        static constexpr std::size_t kCapacity = 4096; // TODO: remove for later phase
        // special value (signature) to prove that the bytes at that address is ours
        static constexpr std::uint32_t kMagic = 0x5A43534D; // hexadecimal for ascii 'ZCSM'

        std::atomic<std::uint64_t> seq;
        std::atomic<std::uint64_t> ack;
        std::atomic<std::uint32_t> ready;
        std::uint32_t len;
        std::byte payload[kCapacity];
    };

    static_assert(
        std::atomic<std::uint64_t>::is_always_lock_free,
        "seq must be lock-free");
    static_assert(std::is_standard_layout_v<MessageHeader>);
    static_assert(sizeof(MessageHeader) == 4120);
    static_assert(offsetof(MessageHeader, payload) == 24);
}
